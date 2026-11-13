// festivals: the world's side of Festival Season (update 4 SPEC v1.7 §2.15.1; F-world's file): the race and its marks, the crown and
// its tickets, the roll and the defender, Discover widened, the saves' festival lines, texts and letters, the Pass's first edition on
// two rents, the agenda merge, the Rerun keys. Called from festivals_main.cpp (its kit: Expect).
// Phase A (§6 PA-48): registered, empty. G1: F-world's checks land package by package (FW-1: FestReignCopy, the strings and the
// sponsor half). Every helper here is in an anonymous namespace (PLAN_G1 §1 item 8: festivals_main.cpp shares the executable).
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sponsor.h"

#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace festivals_test
{
void Expect(bool Condition, const char* What); // festivals_main.cpp (the kit)
void Expect(bool Condition, const std::string& What);

namespace
{
/** ASCII lower case. */
std::string FestLowerWorld(std::string S)
{
	for (char& C : S)
	{
		C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
	}
	return S;
}

/** Whether Word (lower case) stands alone in Low (lower case): no letter or digit just before or after it. */
bool FestHasWordWorld(const std::string& Low, const std::string& Word)
{
	for (size_t At = Low.find(Word); At != std::string::npos; At = Low.find(Word, At + 1))
	{
		const size_t End = At + Word.size();
		const bool Before = At == 0 || !std::isalnum(static_cast<unsigned char>(Low[At - 1]));
		const bool After = End >= Low.size() || !std::isalnum(static_cast<unsigned char>(Low[End]));
		if (Before && After)
		{
			return true;
		}
	}
	return false;
}

/** How often Phrase (lower case) occurs in Low. */
int FestCountWorld(const std::string& Low, const std::string& Phrase)
{
	int N = 0;
	for (size_t At = Low.find(Phrase); At != std::string::npos; At = Low.find(Phrase, At + 1))
	{
		++N;
	}
	return N;
}

/** An inventory entry without its kind ("label\tMICRO MAIN EVENT" -> "MICRO MAIN EVENT"; §2.11: the kinds are lower-case words). */
std::string FestStripKindWorld(const std::string& S)
{
	const size_t Tab = S.find('\t');
	if (Tab == std::string::npos || Tab == 0)
	{
		return S;
	}
	for (size_t I = 0; I < Tab; ++I)
	{
		if (S[I] < 'a' || S[I] > 'z')
		{
			return S;
		}
	}
	return S.substr(Tab + 1);
}

/** Every string FestReignCopy reads (§2.15.1: fest::Inventory, fest::TextInventory, FestUiInventory, FestArtInventory, site::Copy;
 *  the copy table's titles and labels too), kinds stripped, each with the inventory it came from. */
std::vector<std::pair<std::string, std::string>> FestReignStrings()
{
	std::vector<std::pair<std::string, std::string>> Out;
	const auto Add = [&Out](const char* From, const std::vector<std::string>& Lines) {
		for (const std::string& L : Lines)
		{
			Out.emplace_back(From, FestStripKindWorld(L));
		}
	};
	Add("fest::Inventory", ss::fest::Inventory());
	Add("fest::TextInventory", ss::fest::TextInventory());
	Add("FestUiInventory", ss::fest::FestUiInventory());
	Add("FestArtInventory", ss::fest::FestArtInventory());
	Add("site::Copy", ss::site::Copy());
	Add("site::CopyLabels", ss::site::CopyLabels());
	return Out;
}

/** S with every festival placeholder filled as REIGN prints it ({Festival}, {FESTIVAL}, the story line's {A}, {B}, {C} with
 *  "REIGN 2027"; {Short}, {SHORT} with "REIGN"): a template filled that way is a REIGN string too. */
std::string FestReignFillWorld(const std::string& S)
{
	const std::pair<const char*, const char*> Fills[] = {{"{Festival}", "REIGN 2027"}, {"{FESTIVAL}", "REIGN 2027"}, {"{A}", "REIGN 2027"},
		{"{B}", "REIGN 2027"}, {"{C}", "REIGN 2027"}, {"{Short}", "REIGN"}, {"{SHORT}", "REIGN"}};
	std::string Out = S;
	for (const auto& F : Fills)
	{
		const std::string Key = F.first;
		for (size_t At = Out.find(Key); At != std::string::npos; At = Out.find(Key, At))
		{
			Out.replace(At, Key.size(), F.second);
			At += std::string(F.second).size();
		}
	}
	return Out;
}

/** The sponsor half's planted REIGN Main win (WhyLine reads L.LiveEntries, G/Sponsor.cpp: a finished entry won), on a record that
 *  opens Volt Rush (standing 2 by the stream road and six cans), Overclock (300 followers, 15 a night) and Rangewise (a title and five
 *  cashes): the offer letter BrandId sends, unforced ("" when the brand mails none). */
std::string FestReignOfferWorld(const std::string& BrandId)
{
	ss::life::State L;
	L.Bought["volt-rush"] = 6;
	ss::life::LiveEntry Won;
	Won.Id = "runittwice.reign27-main@20";
	Won.Name = "REIGN 30: $2,100 NLHE [Main Event]";
	Won.State = ss::life::LiveEntry::Finished;
	Won.Entrants = 2766;
	Won.Place = 1;
	Won.PrizeCents = 120000000;
	Won.FinishedAt = 20.0 * 1440.0;
	L.LiveEntries.push_back(Won);
	ss::rep::Public P;
	P.Followers = 600;
	P.Avg30 = 25.0;
	P.Titles = 1;
	P.LiveCashes = 5;
	const std::string Id = ss::sponsor::MakeOffer(L.Sponsors, L, P, BrandId, 30.0 * 1440.0, "fest-reign", false);
	for (const ss::life::Letter& M : L.Inbox)
	{
		if (!Id.empty() && M.Kind == ss::sponsor::LetterOffer && M.Thread == Id)
		{
			return M.Body;
		}
	}
	return std::string();
}

/**
 * FestReignCopy (v1.6; §2.15.1, §2.11, APPLY_CHAMPS §5.2). REIGN is RunItTwice's word for its championship, and only that: no string
 * in any festival inventory or the sites' copy holds "reign cabinet"; no REIGN string (one naming REIGN, or a festival template filled
 * as REIGN prints) holds "reigning" (REIGNING stays RiverLine's Player of the Year label) or "crown"; "Long may you reign" prints once,
 * in Dee's T16. The sponsor half: with a REIGN Main win planted, no Energy brand's offer says it (an Energy brand never takes
 * why:title filled with a REIGN event; the guard is the lead's G/Sponsor.cpp patch, fworld_to_lead_2, on a path no brand takes
 * today), the planted win does reach the why line (Rangewise's letter says it), and no Energy brand's read, product or name fills a
 * REIGN name or Short. No REIGN surface drawing a sponsor mark is u4_ui's half (F-hubs, F-look).
 * Planted bugs: "REIGN cabinet" in a TextInventory line; Volt Rush's why line set to why:title (the SPEC's "You won the REIGN Main
 * Event, and people noticed." as a Volt Rush offer); with the lead's patch in, the guard removed under that why line.
 */
void FestReignCopy()
{
	const std::vector<std::pair<std::string, std::string>> Strings = FestReignStrings();
	std::string Cabinet, Reigning, Long;
	int LongCount = 0;
	bool LongInT16 = false;
	int ReignStrings = 0;
	for (const auto& S : Strings)
	{
		const std::string Low = FestLowerWorld(S.second);
		if (Low.find("reign cabinet") != std::string::npos)
		{
			Cabinet += " [" + S.first + "] \"" + S.second + "\"";
		}
		const std::string Filled = FestReignFillWorld(S.second);
		if (FestHasWordWorld(Low, "reign") || Filled != S.second)
		{
			++ReignStrings;
			const std::string FilledLow = FestLowerWorld(Filled);
			if (FilledLow.find("reigning") != std::string::npos || FilledLow.find("crown") != std::string::npos)
			{
				Reigning += " [" + S.first + "] \"" + Filled + "\"";
			}
		}
		const int N = FestCountWorld(Low, "long may you reign");
		if (N > 0)
		{
			LongCount += N;
			Long += " [" + S.first + "] \"" + S.second + "\"";
			LongInT16 = LongInT16 || (S.first == "fest::TextInventory" && S.second.rfind("The Sceptre, kid. ", 0) == 0 && N == 1);
		}
	}
	std::printf("  FestReignCopy: %zu strings read, %d REIGN strings (naming REIGN or filled as REIGN)\n", Strings.size(), ReignStrings);
	Expect(Cabinet.empty(), "FestReignCopy: no inventory string holds \"reign cabinet\"" + (Cabinet.empty() ? std::string() : ":" + Cabinet));
	Expect(Reigning.empty() && ReignStrings > 0, "FestReignCopy: no REIGN string holds \"reigning\" or \"crown\" (" + std::to_string(ReignStrings) + " read)" +
		(Reigning.empty() ? std::string() : ":" + Reigning));
	Expect(LongCount == 1 && LongInT16, "FestReignCopy: \"Long may you reign\" prints once, in Dee's T16 (" + std::to_string(LongCount) + " found:" + Long + ")");

	// The sponsor half.
	std::string Said;
	bool VoltMailed = false;
	int Energy = 0;
	std::string Reads;
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		if (B.Cat != ss::sponsor::Category::Energy)
		{
			continue;
		}
		++Energy;
		const std::string Letter = FestReignOfferWorld(B.Id);
		VoltMailed = VoltMailed || (B.Id == "voltrush" && !Letter.empty());
		if (FestHasWordWorld(FestLowerWorld(Letter), "reign") || Letter.find("You won the") != std::string::npos)
		{
			Said += " [" + B.Id + "] \"" + Letter + "\"";
		}
		for (const std::string& T : {B.Name, B.Short, B.Kast, B.Product, B.ReadLine})
		{
			if (FestHasWordWorld(FestLowerWorld(T), "reign") || T.find('{') != std::string::npos)
			{
				Reads += " [" + B.Id + "] \"" + T + "\"";
			}
		}
	}
	const std::string Control = FestReignOfferWorld("rangewise");
	const bool Reached = Control.rfind("You won the REIGN 30: $2,100 NLHE [Main Event], and people noticed. ", 0) == 0;
	Expect(Energy >= 2 && VoltMailed && Said.empty() && Reached,
		"FestReignCopy: with a REIGN Main win planted, no Energy brand's offer says it (" + std::to_string(Energy) + " Energy brands, Volt Rush's letter " +
			(VoltMailed ? "sent" : "missing") + "; the win reaches Rangewise's why line: " + (Reached ? "yes" : "no") + ")" + (Said.empty() ? std::string() : ":" + Said));
	Expect(Reads.empty(), "FestReignCopy: no Energy brand's name, read or product fills a REIGN name or Short" + (Reads.empty() ? std::string() : ":" + Reads));
}
} // namespace

void SegWorld()
{
	FestReignCopy();
}
} // namespace festivals_test
