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
//
// Update 4 (update 4 SPEC v1.7 §1.2 items 1 and 4, §6 PA-1; Phase A): both tiers gain the user's list of 2026-11-10 (310 source and
// 64 copy entries) and the yearly championships' (70 and 14), pasted as SPEC §6 PA-1 prints them (380 and 78 in all; their
// proofs: durable/update4/model/spec-restore/deny_proof.py and champs/champ_deny_proof.py, 0 hits on the game's strings), plus
// the six DESIGN §2.6 deny entries the printed list doesn't hold (the same proof's "design2.6" group, 0 hits). None matches a name
// the game prints today: RCOP's strings and the six tournament names the game already prints (Sunday Storm, RiverLine Millions,
// Platinum Pass, Friday Night Fight, Flip & Run, The Closer) stay by the user's decisions of 2026-11-09 and 2026-11-10, and no
// entry may ever match them. Two new lists ride beside the tiers: kNameScan, the real-player handle screen (rival_sites'
// HandlesScreened runs it over 20,000 handles::Make names; G2's UniqueName retries on a hit), and kDesignWords, the design words
// that never reach copy (§1.2 item 4, with no allowance), which Update 4's copy checks (FestCopy, copy_lint_world, BigCopy) run
// over the new inventories only.
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
	"joey ingram", "alexandra botez", "hustler casino live", "lodge live",
	// Update 4 (SPEC v1.7 §6 PA-1): the user's list of 2026-11-10 (v1.5's 310), then the yearly championships' 70, as printed.
	// Update 4 names, round 1 (durable/update4/names/RESEARCH_SERIES.md §4.1): real series, festivals, tours, venues and media.
	"wcoop", "tcoop", "njcoop", "njscoop", "secoop", "pacoop", "micoop", "uscoop", "oncoop", "us coop",
	"world championship of online poker", "spring championship of online poker", "turbo championship of online poker", "ftops",
	"mini-ftops", "full tilt online poker series", "ipops", "ipoker online poker series", "uboc", "ultimate bet online championship",
	"championship online poker series", "csop", "crypto series of online poker", "bsop", "brazilian series of poker",
	"zodiac series of poker", "u series of poker", "wsope", "wsopc", "wsop circuit", "super circuit online", "european poker tour",
	"appt", "asia pacific poker tour", "lapt", "latin american poker tour", "napt", "north american poker tour", "ukipt", "espt",
	"estrellas poker tour", "eureka poker tour", "france poker series", "anzpt", "gukpt", "grosvenor", "mid-states poker tour",
	"major series of poker", "heartland poker tour", "rungood poker series", "deepstacks poker tour", "lone star poker series",
	"asian poker tour", "irish poker tour", "sigma poker tour", "celebrity poker tour", "smart poker tour", "balkan poker circuit",
	"888live", "caribbean adventure", "pspc", "global poker league", "super high roller bowl", "shrb", "super high roller series",
	"poker masters", "us poker open", "u.s. poker open", "high rollers week", "poker players championship", "big one for one drop",
	"five diamond", "bellagio", "festa al lago", "aria poker classic", "wynn", "signature series", "legends of poker",
	"tournament of champions", "master classics of poker", "merit poker", "kings of tallinn", "lions of the east", "battle of malta",
	"australian poker open", "aussie millions", "irish open", "irish poker open", "seminole hard rock", "shrpo", "lhpo", "rrpo",
	"lucky hearts", "rock 'n' roll poker open", "rock n roll poker open", "borgata", "la poker classic", "l.a. poker classic",
	"big poker oktober", "bay 101", "deepstack extravaganza", "deepstack championship", "venetian deepstack", "grand poker series",
	"main event mania", "big stax", "superstacks", "foxwoods", "mohegan", "world poker finals", "mirage poker showdown",
	"poker showdown", "bestbet", "potomac poker open", "hollywood poker open", "marrakech poker open", "bar poker open",
	"mix game carnival", "circus poker series", "mediterranean poker party", "caribbean poker party", "poker party", "sismix",
	"cash game festival", "pokercode", "pokernews cup", "hendon mob", "pokernews", "micromillions", "micro millions", "microfestival",
	"micro festival", "galactic series", "stadium series", "coinmasters", "thursday thrill", "sunday supersonic", "sunday kickoff",
	"gg spring festival", "gg world festival", "ggpoker online championship", "ggoc", "ggmasters", "gg masters", "ggmillion$",
	"ggmillions", "super million$", "super millions", "bounty hunters series", "omaholic", "winter giveaway series", "wsop express",
	"thursday throwdown", "monday monster stack", "powerfest", "millions online", "online super series", "venom series", "xl inferno",
	"xl eclipse", "xl blizzard", "super xl", "xl championships", "xl spring series", "xl autumn series", "wptdeepstacks", "wpt500",
	"wpt prime", "betmgm", "bopc", "big shots series", "double trouble series", "summer pko fest", "march poker mania", "monthly milly",
	"black diamond poker open", "golden spade", "twelve days of turbos", "triple header sunday", "mystery millions",
	"millionaire maker", "plossus", "spooky monster stack", "crazy 8's", "triple 3's", "summer splash", "grand prix turbo series",
	"late night poker", "poker after dark", "shark cage", "bluff magazine", "all in magazine", "cardplayer", "card player magazine",
	"ante up poker", "full house resorts",
	// Round 1's shortlist kills (NAMES_SHORTLIST.md §7 note 3; not "back to back", which is copy).
	"festival of felt", "boardwalk series", "straddle club", "first to act poker", "mystery pot luck",
	"check raise poker championships", "neon poker",
	// Round 2 (RESEARCH_GG_PS.md §7.1): PokerStars and GGPoker names not in round 1.
	"nascoop", "pascoop", "miscoop", "nacoop", "wcoop boot camp", "wcoop challenge", "triple coop", "sunday billion",
	"sunday 1/4 million", "daily bigs", "hot turbos", "omania", "saturday line-up", "battle of the planets", "galactic stars battle",
	"galactic games", "fenomeno", "series saver", "series wrap-up", "sunday cooldown", "daily cooldown", "daily supersonic",
	"late night fight", "mystery warm-up", "prize vault", "emerald pass", "power pass", "big game on tour", "ept online",
	"dynasty series", "sunday dynasty", "sunday shield", "bounty dynasty", "bounty storm", "ggs", "ggsf", "ggwf", "ggseries",
	"good game series", "gg world championship", "world bounty festival", "world festival classic", "mystery closer", "gg millions",
	"gg million$", "million$", "omaholics", "omahasquad", "ocean ko", "daily bounty king", "santa's sled", "bounty snowball",
	"holiday heater", "zodiac festival", "wonderbox", "secret ko", "super kickoff", "thursday thriller", "sunday heater",
	"road to vegas", "wsop paradise", "wsopp", "gladiators of poker", "double stack million", "ace poker league", "apl championship",
	"apt online", "flipout", "superpro", "superranking", "ggcare", "overlay edition", "phase-l", "phase-m", "phase-h", "phase-s",
	// Round 2 acronym screen (PROPOSALS2_ACRONYM.md §8).
	"jet set season", "noir poker series", "onyx high roller series", "gambit cup", "snowmen festival", "quads poker championship",
	"nutsr", "nuts poker league", "full flush poker", "hyper dash", "poker jamboree", "posh poker", "runout poker", "final orbit",
	"broadway bets", "opuspoker",
	// Rankings (the user, 2026-11-10: the rankings are the TPI Rankings; GPI is the real Global Poker Index).
	"gpi", "global poker index", "official poker rankings",
	// Round 2 follow-up screen (SCREEN2_FOLLOWUP.md): real gambling products and venues met while screening.
	"european mixed poker championship", "empc", "royal poker tour", "rpt championship", "redeal blackjack", "blackjack redeal",
	"european blackjack redeal gold", "duetqq", "ritual respins", "7 seas casino", "running aces", "kojack's",
	// kSourceScan, after v1.5's 310 (66): the yearly championships (durable/update4/champs/APPLY_CHAMPS.md §5).
	// Real yearly-championship names, routes, boards and prizes (CHAMP_RESEARCH.md §13.1; "platinum pass" left out: it matches the Season board's "Platinum Pass", one of the six names the game keeps).
	"world online championships", "world online championship", "wpt woc", "player of the championship", "wpt rising star",
	"wpt champions club", "mike sexton champions cup", "wpt prime championship", "wpt world championship", "wpt passport",
	"passport dollars", "wsop online", "wsop super circuit online", "super main event", "continental flipout", "continental flipouts",
	"bracelet standing freeroll", "road to paradise", "wsop circuit championship", "mega path", "stars spinner", "scoop warm-up",
	"oss xl", "oss cub3d", "venom fever", "venom madness", "venom vault", "poison path", "survivor flips", "xl winter series",
	"mystery starter", "3 million event", "giant space ko", "space ko", "colossus million", "road to series", "eliminator xxl",
	"six figure sunday", "sunday c-note", "sunday quarter", "may millions", "online summer classic", "gspo",
	"coin series of online poker", "ftops jersey", "big, big series", "hublot",
	// Real names met screening the championship names (SCREEN_CHAMPS.md §5.3).
	"bragg", "bragg gaming", "inclave", "crit gaming agency", "colorado river indian tribes", "bluewater resort", "dice thrice",
	"future anthem", "anthem gaming", "capstone software", "reign total body fuel", "aristocrat",
	// Real names met by the Ignite namer (NAMES_IGNITE.md §4 item 7).
	"throne poker", "sovereign poker", "apex poker", "regal poker", "hiscore games", "throne of fortune", "winthrone",
	// kSourceScan, after the 66 above (4): real names met by champ-screen2 (SCREEN_CHAMPS2.md §2).
	"pine tar poker", "poker concierge", "sceptre of cleo", "bullpen casino",
	// Update 4 (SPEC v1.7 §6 PA-1, DESIGN §2.6's deny-list): DESIGN's entries the printed list doesn't hold (whole phrases, never in copy).
	"ecoop", "high roller club", "estrellas", "unibet open", "pocketfives", "sharkscope",
};

/** Real names that are also ordinary words or fragments: screened over the copy inventories only. The creator update
 *  (SPEC_CREATORS §5.5, Phase A-K) adds the real platforms' feature names its Kast and Rerun copy avoids ("drops" stays out:
 *  it is ordinary copy, and "Live Drop" is screened by hand). Its "spotlight" waits: Ignite's BUZZ says "The spotlight on
 *  {Headliner}'s seat is so dramatic." (RiverLineIgnite.cpp), the word's ordinary sense, which this tier would flag. */
inline constexpr const char* kCopyScan[] = {"kick", "rumble", "twitch", "888", "wsop", "world series", "wpt", "upswing", "bodog", "ignition", "shorts", "premieres",
	"backstage", "run good", "rungood", "streak freeze", "battle pass", "bumper", "ept", "acr", "supernova", "rampage", "wolfgang", "mariano",
	"reels", "shout-out", "shoutout", "hype train", "discovery feed", "play button", "subathon", "channel points", "watch streak", "premiere",
	// Update 4 (SPEC v1.7 §6 PA-1): the user's list of 2026-11-10 (v1.5's 64), then the yearly championships' 14, as printed.
	// Update 4 names, round 1 (RESEARCH_SERIES.md §4.2): real series that are also ordinary words or fragments.
	"scoop", "venom", "the venom", "colossus", "goliath", "moneymaker", "the weekender", "ko series", "turbo series",
	"players championship", "players' championship", "crazy eights", "high stakes poker", "purple jacket", "golden nugget",
	"dusk till dawn", "venetian", "seminole", "spring fling", "oss", "uos", "pca", "hpt", "pgt", "rgps", "mspt", "grand prix",
	"elite poker", "inferno series", "eclipse series", "blizzard series", "carnival series", "circus series", "giveaway series",
	"splash series", "pko fest",
	// Round 1's shortlist kills that are ordinary words (NAMES_SHORTLIST.md §7 note 3).
	"flophouse", "topdeck", "pair pressure",
	// Round 2 (RESEARCH_GG_PS.md §7.2): PokerStars and GGPoker names that are ordinary phrases ("ignite series": never the site's festival).
	"super tuesday", "titans", "the centurion", "copacabana", "grand tour", "lucky dip", "ticket machine", "loose cannon", "boot camp",
	"bounty hunters", "global festival", "winter is coming", "mystery ball", "tick tock", "super monday", "saturday knockout",
	"beat the pros", "big 500", "ignite series", "apl", "wgs",
	// Round 2 follow-up (SCREEN2_FOLLOWUP.md): real products that are ordinary phrases.
	"end of summer series", "mystery knockout", "mystery knockouts", "deep pockets",
	// kCopyScan, after v1.5's 64 (12): ordinary phrases that are real championship names or features.
	"world championship", "ignite championship", "players' choice", "players' choice tournament", "sunday masters", "sunday sweat",
	"summer saver", "baccarat",
	"run it three times", "run it thrice",
	"hi-score", "royal event",
	// kCopyScan, after the 12 above (2): real names that are ordinary phrases ("club championship" never matches "the club's championship").
	"skeleton key", "club championship",
};

/**
 * Update 4 (SPEC v1.7 §1.2 item 1, §6 PA-1): the real-player handle screen. Real people's names in handle form (ASCII lower case,
 * letters and digits only, as HandleKey writes a handle): the real people the two tiers above already name. A generated handle
 * whose key contains one is a hit (FindHandle). rival_sites' HandlesScreened runs it over a 20,000-name sample of handles::Make
 * (and handles::MakeWide once G4 gives it a body); from G2 every handle drawn at run time is checked against the same list
 * (UniqueName retries on a hit). The lead may add names; each must leave the sample clean.
 */
inline constexpr const char* kNameScan[] = {"bradowen", "andrewneeme", "dougpolk", "jonathanlittle", "lexveldhuis", "spraggy", "hungryhorse", "jamanburton",
	"johnnievibes", "kevinmartin", "joeyingram", "alexandrabotez", "moneymaker"};

/**
 * Update 4 (SPEC v1.7 §1.2 item 4, §3.20, §6 PA-1): the design words that never reach copy, with no allowance (§12.1 I-9: FEIGN's
 * headliner is The Big Top). Matched like the tiers (whole words, any case) over Update 4's inventories only: FestCopy and
 * FestCopyUi (G1), copy_lint_world (G2, G4), BigCopy (G3). "autoplay" stays legal in the system UI that already uses it, which no
 * Update 4 inventory holds; today's copy (Kast's Heat, the register strip's autoplay) is not screened by it.
 */
inline constexpr const char* kDesignWords[] = {"whale", "crowd", "shadow", "followed", "promoted", "tier", "growth", "surge", "slump", "director", "heat",
	"micro-mega", "autoplay"};

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

/** A handle as kNameScan compares it: ASCII lower case, letters and digits only ("Doug_Polk88" -> "dougpolk88"). */
inline std::string HandleKey(const std::string& Handle)
{
	std::string Key;
	for (const char C : Handle)
	{
		if (std::isalnum(static_cast<unsigned char>(C)))
		{
			Key += static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
		}
	}
	return Key;
}

/** The first kNameScan name a handle holds (its key contains the entry), or nullptr. */
inline const char* FindHandle(const std::string& Handle)
{
	const std::string Key = HandleKey(Handle);
	for (const char* N : kNameScan)
	{
		if (Key.find(N) != std::string::npos)
		{
			return N;
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
