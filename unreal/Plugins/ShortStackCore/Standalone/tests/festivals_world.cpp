// festivals: the world's side of Festival Season (update 4 SPEC v1.7 §2.15.1; F-world's file): the race and its marks, the crown and
// its tickets, the roll and the defender, Discover widened, the saves' festival lines, texts and letters, the Pass's first edition on
// two rents, the agenda merge, the Rerun keys. Called from festivals_main.cpp (its kit: Expect).
// Phase A (§6 PA-48): registered, empty. G1: F-world's checks land package by package (FW-1: FestReignCopy, the strings and the
// sponsor half; FW-2a: the race and its marks: FestRaceDeterministic, FestRaceTiesToPerson, FestMarksNested, FestLadderCutoff,
// FestReloadMidRace, FestOldSaveMidFestival and FestSaveRoundTrip's race part, with the race's budget lines). Every helper here is in an
// anonymous namespace (PLAN_G1 §1 item 8: festivals_main.cpp shares the executable); each check runs alone as "festivals_test <Check>"
// (the kit's Only, FN-1). The race checks build one Blank world before RCOP 2026 opens and run it through the edition (about 18 ms a
// world day), planting races through the world text, never simulating past RCOP (§8.1).
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldSim.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace festivals_test
{
void Expect(bool Condition, const char* What); // festivals_main.cpp (the kit)
void Expect(bool Condition, const std::string& What);
bool Only(const char* Check);                   // festivals_main.cpp (the one-check filter, FN-1)

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

// ------------------------------------------------------------------ FW-2a: the race and its marks (§2.5, §2.12, §2.13, §2.14)

/** The race checks' world seed. */
constexpr uint32_t kFestSeedWorld = 0x2a2026u;
/** Every row of a race (FestTop's count). */
constexpr int kFestAllWorld = 1 << 30;

/** A Blank world made before RCOP 2026 opens (day 12, 06:00; RCOP's race opens at day 13's midnight): the race checks' start. */
ss::world::World FestFreshWorld()
{
	ss::world::World W;
	W.Create(kFestSeedWorld, 12.0 * 1440.0 + 360.0, ss::world::Start::Blank);
	return W;
}

/** A world's save text (its world lines, as the save writes them). */
std::string FestTextWorld(const ss::world::World& W)
{
	std::string Out;
	W.Write(Out);
	return Out;
}

/** A world read back from save text: each line's fields split on tabs, then Finish (the load's order). */
void FestLoadWorld(ss::world::World& W, const std::string& Text)
{
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End;
		std::vector<std::string> Fields;
		size_t From = Start;
		for (size_t K = Start; K <= End; ++K)
		{
			if (K == End || Text[K] == '\t')
			{
				Fields.push_back(Text.substr(From, K - From));
				From = K + 1;
			}
		}
		W.Read(Fields);
		Start = End + 1;
	}
	W.Finish();
}

/** Text's lines that start with Prefix, in order, each ending in '\n'; Keep false: every other line instead. */
std::string FestLinesWorld(const std::string& Text, const std::string& Prefix, bool Keep = true)
{
	std::string Out;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End;
		if ((Text.compare(Start, Prefix.size(), Prefix) == 0) == Keep)
		{
			Out.append(Text, Start, End - Start);
			Out += '\n';
		}
		Start = End + 1;
	}
	return Out;
}

/** Two races row for row (npc, points, cashes). */
bool FestSameRaceWorld(const std::vector<std::pair<int, ss::world::FestScore>>& A, const std::vector<std::pair<int, ss::world::FestScore>>& B)
{
	if (A.size() != B.size())
	{
		return false;
	}
	for (size_t K = 0; K < A.size(); ++K)
	{
		if (A[K].first != B[K].first || A[K].second.Points != B[K].second.Points || A[K].second.Cashes != B[K].second.Cashes)
		{
			return false;
		}
	}
	return true;
}

/** "rcop" -> its edition (RCOP 2026, days 13-34). */
const ss::fest::Festival* FestRcopWorld()
{
	return ss::fest::FestivalOf("rcop");
}

/** The hero's RCOP 2026 event: the first numbered event (no satellite) starting on its day 14 with a field of 50 or more. */
bool FestHeroEventWorld(ss::net::EventInstance& Out)
{
	const ss::net::Network& N = ss::net::Shared();
	for (const ss::net::EventInstance& E : N.Window(14.0 * 1440.0, 15.0 * 1440.0))
	{
		const ss::net::EventTemplate& T = N.TemplateOf(E);
		if (T.Series == "rcop" && T.Fmt != ss::net::Format::Satellite && E.Entries >= 50)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

/** The hero plays E: registered at its start, first of its field, the result told twice (World::HeroFinished: one result). */
void FestHeroPlaysWorld(ss::world::World& W, const ss::net::EventInstance& E)
{
	W.AdvanceTo(E.Start);
	W.HeroEntered(E.Id, std::string());
	const ss::world::World::TableReport Report;
	W.HeroFinished(E.Id, 1, E.Pool / 5, Report);
	W.HeroFinished(E.Id, 1, E.Pool / 5, Report);
}

/** The peak world's build (FestPeakWorld) and what its run read. */
struct FestPeakRunWorld
{
	std::unique_ptr<ss::world::World> W;
	double Ms = 0.0;      // the run, day 12 to LastEnd
	int ViewReads = 0;    // the days the views were read on the way
	bool Hero = false;    // the hero's event was found and played
};

/** RCOP 2026's race at its peak: the fresh world run to fest::LastEnd (its last event's end; FW-2b's crown comes at the next hour), a
 *  day at a time with the views read each day (the cache must follow the race), the hero's day-14 result told twice. Built once. */
FestPeakRunWorld& FestPeakWorld()
{
	static FestPeakRunWorld Run;
	if (!Run.W)
	{
		const auto T0 = std::chrono::steady_clock::now();
		Run.W = std::make_unique<ss::world::World>(FestFreshWorld());
		ss::world::World& W = *Run.W;
		const ss::fest::Festival* Rcop = FestRcopWorld();
		ss::net::EventInstance E;
		Run.Hero = Rcop && FestHeroEventWorld(E);
		const double End = Rcop ? ss::fest::LastEnd(*Rcop) : 36.0 * 1440.0;
		for (int Day = 13; static_cast<double>(Day) * 1440.0 < End; ++Day)
		{
			if (Run.Hero && Day == 14)
			{
				FestHeroPlaysWorld(W, E); // to its start on day 14, before the day's view read
			}
			W.AdvanceTo(static_cast<double>(Day) * 1440.0 + 600.0);
			Run.ViewReads += ss::world::FestTop(W, 10).empty() && ss::world::FestKnown(W).empty() ? 0 : 1;
			(void)ss::fest::MarkCutoff(W, ss::world::FestMark::Finalist);
		}
		W.AdvanceTo(End);
		Run.Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	}
	return Run;
}

/** RCOP 2026 mid-race: the fresh world run to its day 20, 12:00 in one step. Built once. */
const ss::world::World& FestMidWorld()
{
	static std::unique_ptr<ss::world::World> Mid;
	if (!Mid)
	{
		Mid = std::make_unique<ss::world::World>(FestFreshWorld());
		Mid->AdvanceTo(20.0 * 1440.0 + 720.0);
	}
	return *Mid;
}

/**
 * FestRaceDeterministic (§2.15.1, §2.12, §2.5.2 rules 2-3): two worlds from one seed through RCOP 2026, one stepped a day at a time with
 * its views read each day and one in a single step, hold identical races; the hero's result, told twice, scores once (one cash, its
 * RacePoints); a satellite scores nothing (a planted RCOP satellite win adds no row; the same finish as a numbered event does); the
 * views read on the way equal a copy's, ranked fresh (World's FestViews follow the race, R-10).
 * Planted bugs: the Again guard removed (G/WorldHero.cpp: the hero's result scored each time it is told); the cache kept across a race
 * write.
 */
void FestRaceDeterministic()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	ss::net::EventInstance E;
	const bool Found = Rcop && FestHeroEventWorld(E);
	Expect(Found, "FestRaceDeterministic: RCOP 2026 is in fest::All() and runs a numbered event of 50+ on its day 14");
	if (!Found)
	{
		return;
	}
	FestPeakRunWorld& Peak = FestPeakWorld();
	const world::World& A = *Peak.W;
	world::World B = FestFreshWorld();
	FestHeroPlaysWorld(B, E);
	B.AdvanceTo(ss::fest::LastEnd(*Rcop));
	const auto TopA = world::FestTop(A, kFestAllWorld);
	const auto TopB = world::FestTop(B, kFestAllWorld);
	const std::string LinesA = FestLinesWorld(FestTextWorld(A), "world\tfest\t");
	const std::string LinesB = FestLinesWorld(FestTextWorld(B), "world\tfest\t");
	Expect(TopA.size() >= 100 && FestSameRaceWorld(TopA, TopB) && LinesA == LinesB && world::FestRaceKey(A) == "rcop",
		"FestRaceDeterministic: two worlds from one seed through RCOP 2026 (a day at a time, and in one step) hold identical races (" +
			std::to_string(TopA.size()) + " and " + std::to_string(TopB.size()) + " scorers; race lines " + (LinesA == LinesB ? "equal" : "differ") + ")");

	const ss::net::Network& N = ss::net::Shared();
	const ss::net::EventTemplate& T = N.TemplateOf(E);
	const double Want = ss::fest::RacePoints(1, E.Entries, T.BuyInCents, T.Curve);
	world::FestScore Hero;
	for (const auto& Row : TopA)
	{
		Hero = Row.first == -1 ? Row.second : Hero;
	}
	Expect(Want > 0.0 && Hero.Cashes == 1 && Hero.Points == Want && world::FestStanding(A, -1).first > 0,
		"FestRaceDeterministic: the hero's result, told twice, scores once (" + std::to_string(Hero.Cashes) + " cash(es), " +
			std::to_string(static_cast<long long>(Hero.Points)) + " pts; want 1 and " + std::to_string(static_cast<long long>(Want)) + ", " + E.Id + ")");

	const world::World Fresh = A; // a copy starts without views: ranked fresh
	const auto KnownA = world::FestKnown(A);
	const auto KnownF = world::FestKnown(Fresh);
	Expect(Peak.ViewReads >= 15 && FestSameRaceWorld(world::FestTop(Fresh, kFestAllWorld), TopA) && KnownA == KnownF &&
			   world::FestStanding(Fresh, -1) == world::FestStanding(A, -1) &&
			   ss::fest::MarkCutoff(Fresh, world::FestMark::Top10) == ss::fest::MarkCutoff(A, world::FestMark::Top10),
		"FestRaceDeterministic: the views read on each of " + std::to_string(Peak.ViewReads) + " days equal a copy's, ranked fresh (" +
			std::to_string(KnownA.size()) + " people you know)");

	// A satellite never scores (§2.5.2 rule 2): a planted win in an RCOP satellite, then the same finish as a numbered event.
	world::World C = A;
	int Id = 0;
	while (C.Get(Id) && world::FestStanding(C, Id).first != 0)
	{
		++Id;
	}
	world::Pending Sat;
	Sat.Online = true;
	Sat.Series = Rcop->SeriesId;
	Sat.Entries = 300;
	Sat.BuyIn = 21500;
	Sat.Format = static_cast<int>(ss::net::Format::Satellite);
	Sat.Kinds = world::KindSatellite;
	Sat.Ticket = Rcop->MainId;
	world::FestScoreResult(C, Sat, Id, 1);
	const int AfterSat = world::FestStanding(C, Id).first;
	world::Pending Num = Sat;
	Num.Format = static_cast<int>(ss::net::Format::Freezeout);
	Num.Kinds = 0;
	Num.Ticket.clear();
	world::FestScoreResult(C, Num, Id, 1);
	const int AfterNum = world::FestStanding(C, Id).first;
	Expect(C.Get(Id) && AfterSat == 0 && AfterNum > 0,
		"FestRaceDeterministic: a satellite scores nothing (npc " + std::to_string(Id) + ": rank " + std::to_string(AfterSat) +
			" after an RCOP satellite win, " + std::to_string(AfterNum) + " after the same finish in a numbered event)");
}

/**
 * FestRaceTiesToPerson (§2.15.1, §2.5.2 rule 5): FestRank's total order on planted ties: points descending; a person ahead of the hero
 * at equal points (the hero must be strictly ahead); among people, the lower id.
 * Planted bug: the hero winning ties.
 */
void FestRaceTiesToPerson()
{
	namespace world = ss::world;
	world::FestRace R;
	R.Key = "rcop";
	R.Scores[-1] = {100.0, 2};
	R.Scores[9] = {100.0, 1};
	R.Scores[4] = {100.0, 3};
	R.Scores[12] = {140.0, 2};
	R.Scores[2] = {60.0, 1};
	R.Scores[7] = {100.0, 1};
	const auto Order = world::FestRank(R);
	std::string Got;
	for (const auto& Row : Order)
	{
		Got += (Got.empty() ? "" : " ") + std::to_string(Row.first);
	}
	Expect(Got == "12 4 7 9 -1 2", "FestRaceTiesToPerson: a tie ranks every person above the hero, then the lower id (order " + Got + "; want 12 4 7 9 -1 2)");
	world::FestRace Two;
	Two.Scores[-1] = {55.0, 1};
	Two.Scores[1500] = {55.0, 1};
	const auto Pair = world::FestRank(Two);
	Expect(Pair.size() == 2 && Pair[0].first == 1500 && Pair[1].first == -1, "FestRaceTiesToPerson: the hero tied with one person ranks second");
}

/** The SPEC's rule written out independently (§2.5.3): the last rank of Mark or better in a race of S (whole-number ceilings). */
int FestWantLastWorld(ss::world::FestMark Mark, int S)
{
	namespace world = ss::world;
	if (S <= 0 || Mark == world::FestMark::None || Mark == world::FestMark::Count)
	{
		return 0;
	}
	const int Top10 = (S + 9) / 10;
	const int Top25 = (S + 3) / 4;
	const int Kf = S >= 250 ? std::min(25, std::max(3, Top10)) : std::min(10, std::max(3, (S + 19) / 20));
	int Last = 1;
	Last = Mark <= world::FestMark::Podium ? std::max(Last, 3) : Last;
	Last = Mark <= world::FestMark::Finalist ? std::max(Last, Kf) : Last;
	Last = Mark <= world::FestMark::Top10 ? std::max(Last, Top10) : Last;
	Last = Mark <= world::FestMark::Top25 ? std::max(Last, Top25) : Last;
	return std::min(Last, S);
}

const char* FestMarkNameWorld(ss::world::FestMark M)
{
	switch (M)
	{
	case ss::world::FestMark::None: return "none";
	case ss::world::FestMark::Top25: return "TOP 25%";
	case ss::world::FestMark::Top10: return "TOP 10%";
	case ss::world::FestMark::Finalist: return "FINALIST";
	case ss::world::FestMark::Podium: return "PODIUM";
	case ss::world::FestMark::Title: return "the title";
	case ss::world::FestMark::Count: break;
	}
	return "?";
}

/**
 * FestMarksNested (§2.15.1, §2.5.3): for every race of S = 1..3,000 scorers, MarkFor is non-increasing in rank and each grade's ranks
 * are contiguous; FestLastRank (the ladder's cut-off rank) is the last rank meeting each grade and equals the SPEC's rule written out;
 * FINALIST is never narrower than the podium and never wider than the top 10% once that is 3 or more; S = 640 gives FINALIST to the
 * top 25, S = 110 to the top 6 (TOP 10% to the top 11), S = 25 to the top 3.
 * Planted bug: FINALIST fixed at 25.
 */
void FestMarksNested()
{
	namespace world = ss::world;
	int Bad = 0;
	std::string First;
	for (int S = 1; S <= 3000; ++S)
	{
		int Prev = static_cast<int>(world::FestMark::Count);
		std::vector<int> Last(static_cast<size_t>(world::FestMark::Count), 0);
		for (int R = 1; R <= S; ++R)
		{
			const int M = static_cast<int>(world::MarkFor(R, S));
			if (M > Prev)
			{
				++Bad;
				First = First.empty() ? "S " + std::to_string(S) + ": rank " + std::to_string(R) + " above rank " + std::to_string(R - 1) : First;
			}
			Prev = M;
			for (int G = 1; G <= M; ++G)
			{
				Last[static_cast<size_t>(G)] = R; // monotone, so the ranks meeting G or better are 1..Last[G]: contiguous
			}
		}
		for (int G = 1; G < static_cast<int>(world::FestMark::Count); ++G)
		{
			const world::FestMark Mark = static_cast<world::FestMark>(G);
			if (world::FestLastRank(Mark, S) != Last[static_cast<size_t>(G)] || Last[static_cast<size_t>(G)] != FestWantLastWorld(Mark, S))
			{
				++Bad;
				First = First.empty() ? "S " + std::to_string(S) + ": " + FestMarkNameWorld(Mark) + " ends at rank " + std::to_string(Last[static_cast<size_t>(G)]) +
							 " (FestLastRank " + std::to_string(world::FestLastRank(Mark, S)) + ", the rule " + std::to_string(FestWantLastWorld(Mark, S)) + ")"
									  : First;
			}
		}
		const int Kf = world::FestFinalistRanks(S);
		const int Top10 = (S + 9) / 10;
		if (Kf < 3 || (Top10 >= 3 && Kf > Top10))
		{
			++Bad;
			First = First.empty() ? "S " + std::to_string(S) + ": FINALIST " + std::to_string(Kf) + " wide, TOP 10% " + std::to_string(Top10) : First;
		}
	}
	Expect(Bad == 0, "FestMarksNested: for S = 1..3,000 the mark is non-increasing in rank, each grade's ranks contiguous and as the rule (" +
						 std::to_string(Bad) + " problem(s)" + (First.empty() ? std::string() : "; first: " + First) + ")");
	const auto At = [](int R, int S) { return world::MarkFor(R, S); };
	Expect(At(25, 640) == world::FestMark::Finalist && At(26, 640) == world::FestMark::Top10 && At(64, 640) == world::FestMark::Top10 &&
			   At(65, 640) == world::FestMark::Top25 && At(160, 640) == world::FestMark::Top25 && At(161, 640) == world::FestMark::None,
		"FestMarksNested: S = 640 gives FINALIST to the top 25 (TOP 10% to 64, TOP 25% to 160)");
	Expect(At(6, 110) == world::FestMark::Finalist && At(7, 110) == world::FestMark::Top10 && At(11, 110) == world::FestMark::Top10 &&
			   At(12, 110) == world::FestMark::Top25,
		"FestMarksNested: S = 110 gives FINALIST to the top 6 and TOP 10% to the top 11");
	Expect(At(1, 25) == world::FestMark::Title && At(3, 25) == world::FestMark::Podium && world::FestLastRank(world::FestMark::Finalist, 25) == 3 &&
			   At(4, 25) == world::FestMark::Top25 && At(0, 25) == world::FestMark::None && At(26, 25) == world::FestMark::None,
		"FestMarksNested: S = 25 gives FINALIST or better to the top 3 (no rank outside 1..S has a mark)");
}

/** A race line for records "npc:points:cashes" (the save's world\tfest\trace form, points whole). */
std::string FestRaceLineWorld(const std::vector<std::pair<int, int>>& Rows)
{
	std::string Line = "world\tfest\trace\trcop\t";
	for (size_t K = 0; K < Rows.size(); ++K)
	{
		Line += (K ? "," : "") + std::to_string(Rows[K].first) + ":" + std::to_string(Rows[K].second) + ":1";
	}
	return Line + "\n";
}

/**
 * FestLadderCutoff (§2.15.1, §2.5.3, U27; D5): on races planted through the world text, fest::MarkCutoff is the points of the scorer
 * holding each mark's last rank right now, and the ladder's N (FestPointsTo) is that minus the hero's points plus one: 640 scorers with
 * the hero 40th; 110 with the hero tied with the 6th (a person ahead on a tie, so FINALIST is 1 point away); a mark the hero holds reads
 * 0. Before the hero's first cash every rung shows no number (CASH 1 is lit: −1) while the cut-offs stand; with no race, none.
 * Planted bug: the next rank's points used.
 */
void FestLadderCutoff()
{
	namespace world = ss::world;
	const world::World Fresh = FestFreshWorld();
	const std::string Base = FestTextWorld(Fresh);
	const world::FestMark Marks[] = {world::FestMark::Title, world::FestMark::Podium, world::FestMark::Finalist, world::FestMark::Top10, world::FestMark::Top25};
	bool NoRace = world::FestRaceKey(Fresh).empty();
	for (const world::FestMark M : Marks)
	{
		NoRace = NoRace && ss::fest::MarkCutoff(Fresh, M) == 0.0 && world::FestPointsTo(Fresh, M) == -1;
	}
	Expect(NoRace, "FestLadderCutoff: with no race running every cut-off is 0 and no rung shows a number");

	// 640 scorers, rank r holding 5000 - 5r points (ids reversed against the ranks), the hero 40th.
	std::vector<std::pair<int, int>> Rows;
	for (int R = 1; R <= 640; ++R)
	{
		Rows.emplace_back(R == 40 ? -1 : 700 - R, 5000 - 5 * R);
	}
	int RivalId = -1; // the player's rival (the reserved name) scores 100th
	for (int Id = 0; RivalId < 0 && Id < static_cast<int>(Fresh.People().size()); ++Id)
	{
		RivalId = Fresh.Get(Id) && Fresh.Get(Id)->Rival ? Id : -1;
	}
	if (RivalId >= 0 && (RivalId < 60 || RivalId > 699))
	{
		Rows[99].first = RivalId;
	}
	world::World Big;
	FestLoadWorld(Big, Base + FestRaceLineWorld(Rows));
	std::string Got;
	bool Cut = world::FestRaceKey(Big) == "rcop" && world::FestStanding(Big, -1) == std::make_pair(40, 640);
	for (const world::FestMark M : Marks)
	{
		const int Last = FestWantLastWorld(M, 640);
		const double Want = 5000.0 - 5.0 * Last;
		const int WantN = Last >= 40 ? 0 : static_cast<int>(Want) - 4800 + 1;
		const double Have = ss::fest::MarkCutoff(Big, M);
		const int N = world::FestPointsTo(Big, M);
		Cut = Cut && Have == Want && N == WantN;
		Got += std::string(Got.empty() ? "" : "; ") + FestMarkNameWorld(M) + " " + std::to_string(static_cast<long long>(Have)) + "/" +
			   std::to_string(static_cast<long long>(Want)) + " N " + std::to_string(N) + "/" + std::to_string(WantN);
	}
	Expect(Cut, "FestLadderCutoff: 640 scorers, the hero 40th at 4800 pts: each cut-off is its last rank's points and N = cut-off - 4800 + 1 (" + Got + ")");

	// PEOPLE YOU KNOW on the same race (§2.5.4): the rival and the scorers whose bond holds a memory, race order, with their rank in the
	// full race; a memory with someone outside the race adds nobody. (The memories move the world's HeroRev: the views follow.)
	const auto KnownBefore = world::FestKnown(Big);
	Big.Remember(690, world::MemoryKind::BackRoom, "Dee's", 0, Big.Clock());
	Big.Remember(1000, world::MemoryKind::BackRoom, "Dee's", 0, Big.Clock());
	std::vector<std::pair<int, int>> WantKnown;
	for (int R = 1; R <= 640; ++R)
	{
		const int Id = Rows[static_cast<size_t>(R - 1)].first;
		const ss::world::Npc* P = Id >= 0 ? Big.Get(Id) : nullptr;
		if (P && (P->Rival || Id == 690))
		{
			WantKnown.emplace_back(Id, R);
		}
	}
	const auto Known = world::FestKnown(Big);
	std::string KnownText;
	for (const auto& K : Known)
	{
		KnownText += (KnownText.empty() ? "" : ", ") + std::to_string(K.first) + " " + std::to_string(K.second) + "th";
	}
	std::printf("  FestLadderCutoff: people you know in the planted race: %s (before the memory: %zu)\n", KnownText.c_str(), KnownBefore.size());
	const bool RivalIn = std::find_if(Known.begin(), Known.end(), [RivalId](const std::pair<int, int>& K) { return K.first == RivalId; }) != Known.end();
	Expect(RivalIn && Known == WantKnown && KnownBefore.size() + 1 == Known.size() && Known.size() == 2,
		"FestLadderCutoff: PEOPLE YOU KNOW reads the rival and the scorers with a memory, race order with their rank (" + KnownText + ")");

	// 110 scorers, rank r holding 3000 - 10r; the hero tied with the 6th: the person stays ahead, so FINALIST (top 6) is 1 point away.
	std::vector<std::pair<int, int>> Small;
	for (int R = 1; R <= 109; ++R)
	{
		Small.emplace_back(R, 3000 - 10 * R);
	}
	Small.emplace_back(-1, 2940);
	world::World Tie;
	FestLoadWorld(Tie, Base + FestRaceLineWorld(Small));
	const int TieFinalist = world::FestPointsTo(Tie, world::FestMark::Finalist);
	const int TieTop10 = world::FestPointsTo(Tie, world::FestMark::Top10);
	Expect(world::FestStanding(Tie, -1) == std::make_pair(7, 110) && ss::fest::MarkCutoff(Tie, world::FestMark::Finalist) == 2940.0 && TieFinalist == 1 &&
			   ss::fest::MarkCutoff(Tie, world::FestMark::Top10) == 2900.0 && TieTop10 == 0,
		"FestLadderCutoff: 110 scorers, the hero tied with the 6th ranks 7th: FINALIST (2,940) is 1 pt away, TOP 10% (2,900) held (N " +
			std::to_string(TieFinalist) + " and " + std::to_string(TieTop10) + ")");

	// Before the hero's first cash: the same 640 without the hero (a person in the 40th place).
	Rows[39].first = 5;
	world::World NoCash;
	FestLoadWorld(NoCash, Base + FestRaceLineWorld(Rows));
	bool Blank = world::FestStanding(NoCash, -1) == std::make_pair(0, 640) && ss::fest::MarkCutoff(NoCash, world::FestMark::Finalist) == 4875.0;
	for (const world::FestMark M : Marks)
	{
		Blank = Blank && world::FestPointsTo(NoCash, M) == -1;
	}
	Expect(Blank, "FestLadderCutoff: before the hero's first cash CASH 1 is lit with no number (no rung's N; the hero unranked of 640; FINALIST's cut-off 4,875 stands)");
}

/**
 * FestReloadMidRace (§2.15.1, §2.12 reload invariance): a save in the middle of RCOP 2026's race (day 20, 12:00) reloads with its race
 * (the key, every row's points and cashes) and advances to day 24 to the same world text, byte for byte, and the same race as the
 * unbroken run. The save's snapshot (World::Snapshot, Q-13; R-10's copies start without views) writes the saved text and keeps reading
 * its frozen race while the world scores on.
 * Planted bug: world\tfest\trace dropping cashes.
 */
void FestReloadMidRace()
{
	namespace world = ss::world;
	world::World Run = FestMidWorld();
	const std::string Saved = FestTextWorld(Run);
	world::World Back;
	FestLoadWorld(Back, Saved);
	const auto At20 = world::FestTop(Run, kFestAllWorld);
	Expect(world::FestRaceKey(Back) == "rcop" && At20.size() >= 50 && FestSameRaceWorld(world::FestTop(Back, kFestAllWorld), At20),
		"FestReloadMidRace: a save on RCOP's day 20 reloads with its race (" + std::to_string(At20.size()) + " scorers, every row's points and cashes)");

	const std::shared_ptr<const world::World> Snap = Run.Snapshot();
	const auto SnapAt20 = Snap ? world::FestTop(*Snap, kFestAllWorld) : std::vector<std::pair<int, world::FestScore>>();
	Run.AdvanceTo(24.0 * 1440.0);
	Back.AdvanceTo(24.0 * 1440.0);
	const std::string RunText = FestTextWorld(Run);
	const std::string BackText = FestTextWorld(Back);
	const auto At24 = world::FestTop(Run, kFestAllWorld);
	Expect(RunText == BackText && FestSameRaceWorld(world::FestTop(Back, kFestAllWorld), At24),
		"FestReloadMidRace: the reloaded world advances to day 24 as the unbroken run: the same world text (" + std::to_string(RunText.size()) + " and " +
			std::to_string(BackText.size()) + " B, race lines " +
			(FestLinesWorld(RunText, "world\tfest\t") == FestLinesWorld(BackText, "world\tfest\t") ? "equal" : "differ") + ") and race (" +
			std::to_string(At24.size()) + " scorers)");
	Expect(Snap && FestTextWorld(*Snap) == Saved && FestSameRaceWorld(SnapAt20, At20) && FestSameRaceWorld(world::FestTop(*Snap, kFestAllWorld), At20) &&
			   At24.size() > At20.size(),
		"FestReloadMidRace: the save's snapshot writes the saved text and keeps its own race (" + std::to_string(SnapAt20.size()) + " scorers) while the world scores on (" +
			std::to_string(At24.size()) + ")");
}

/**
 * FestOldSaveMidFestival (§2.15.1, §2.5.5, §2.13 old saves): a world written without festival lines (an older build's save) and loaded
 * on RCOP 2026's day 20 has no race this edition: none on load, none after four days, no world\tfest line written; the next edition
 * opens normally (FestDay on its FirstDay, as NewDay calls it).
 * Planted bug: back-filling the race on load.
 */
void FestOldSaveMidFestival()
{
	namespace world = ss::world;
	const world::World& Mid = FestMidWorld();
	const std::string Old = FestLinesWorld(FestTextWorld(Mid), "world\tfest\t", false);
	world::World Loaded;
	FestLoadWorld(Loaded, Old);
	const bool NoneOnLoad = world::FestRaceKey(Mid) == "rcop" && world::FestRaceKey(Loaded).empty() && world::FestTop(Loaded, 1).empty();
	Loaded.AdvanceTo(24.0 * 1440.0);
	const bool NoneLater = world::FestRaceKey(Loaded).empty() && world::FestTop(Loaded, 1).empty() && FestLinesWorld(FestTextWorld(Loaded), "world\tfest\t").empty();
	Expect(NoneOnLoad && NoneLater, std::string("FestOldSaveMidFestival: a day-20 world saved without festival lines has no race this edition (on load: ") +
										(NoneOnLoad ? "none" : "one") + "; four days on: " + (NoneLater ? "none" : "one") + ")");
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Next = Rcop ? ss::fest::Next(Rcop->LastDay) : nullptr;
	world::World Later = Loaded;
	if (Next)
	{
		world::FestDay(Later, Next->FirstDay);
	}
	Expect(Next && world::FestRaceKey(Later) == Next->SeriesId && world::FestTop(Later, 1).empty(),
		"FestOldSaveMidFestival: the next edition opens normally (" + std::string(Next ? Next->Name : "none") + " on its first day: race '" + world::FestRaceKey(Later) + "')");
}

/** The race's budget lines (§2.14; printed, PLAN_FWORLD FW-2's budgets): scoring finishes over RCOP 2026 (one map update each), the
 *  view's ranking and a cached read; the race line's bytes are asserted in FestSaveRoundTrip. */
void FestRaceBudgetsWorld(const ss::world::World& Peak, size_t LineBytes, double RunMs)
{
	namespace world = ss::world;
	const auto Top = world::FestTop(Peak, kFestAllWorld);
	long long Finishes = 0;
	for (const auto& Row : Top)
	{
		Finishes += Row.second.Cashes;
	}
	const world::World Copy = Peak;
	const auto T0 = std::chrono::steady_clock::now();
	(void)world::FestStanding(Copy, -1); // ranks the race (a copy starts without views)
	const auto T1 = std::chrono::steady_clock::now();
	for (int K = 0; K < 1000; ++K)
	{
		(void)ss::fest::MarkCutoff(Copy, world::FestMark::Finalist);
	}
	const auto T2 = std::chrono::steady_clock::now();
	std::printf("  race (RCOP 2026, %d regulars): %lld scoring finishes, one map update each, %zu scorers; the race line %zu B (cap 7 KB); "
				"the view ranked in %.3f ms, a cached read %.3f us; the world run day 12 -> LastEnd %.0f ms\n",
		static_cast<int>(Peak.People().size()), Finishes, Top.size(), LineBytes, std::chrono::duration<double, std::milli>(T1 - T0).count(),
		std::chrono::duration<double, std::micro>(T2 - T1).count() / 1000.0, RunMs);
}

/**
 * FestSaveRoundTrip, the race part (§2.15.1, §2.13; FW-2b adds the curve field on day 66 and the final line's life, FW-7 the fixture):
 * nothing is written when empty (a fresh world writes no world\tfest line, and its text reloads to the same bytes); RCOP 2026's race
 * at its peak writes one race line, at most 7 KB at this world's regulars (its bytes printed), and the world's text reloads to the
 * same bytes; the three world\tfest sub-lines (race, final, mark with and without its unpaid ticket's t) round-trip exactly, and a
 * mark past Count reads None (R-23).
 * Planted bug: the race line written when empty.
 */
void FestSaveRoundTrip()
{
	namespace world = ss::world;
	const world::World Fresh = FestFreshWorld();
	const std::string FreshText = FestTextWorld(Fresh);
	world::World FreshBack;
	FestLoadWorld(FreshBack, FreshText);
	Expect(FestLinesWorld(FreshText, "world\tfest\t").empty() && FestTextWorld(FreshBack) == FreshText,
		"FestSaveRoundTrip: an untouched world writes no world\\tfest line and reloads to the same bytes (" + std::to_string(FreshText.size()) + " B)");

	FestPeakRunWorld& Run = FestPeakWorld();
	const world::World& Peak = *Run.W;
	const std::string PeakText = FestTextWorld(Peak);
	const std::string Race = FestLinesWorld(PeakText, "world\tfest\trace\t");
	const size_t Lines = static_cast<size_t>(std::count(Race.begin(), Race.end(), '\n'));
	const size_t Bytes = Race.empty() ? 0 : Race.size() - 1;
	FestRaceBudgetsWorld(Peak, Bytes, Run.Ms);
	Expect(Lines == 1 && Bytes > 0 && Bytes <= 7000 && FestLinesWorld(PeakText, "world\tfest\t") == Race,
		"FestSaveRoundTrip: RCOP 2026's race at its peak writes one race line of at most 7 KB (" + std::to_string(Lines) + " line(s), " + std::to_string(Bytes) + " B)");
	world::World PeakBack;
	FestLoadWorld(PeakBack, PeakText);
	Expect(FestTextWorld(PeakBack) == PeakText, "FestSaveRoundTrip: the peak world's text reloads to the same bytes (" + std::to_string(PeakText.size()) + " B)");

	const std::string Final = "world\tfest\tfinal\trcop\t50460\t608\t12:1834:9,-1:1700:8,40:1650:7\n";
	const std::string MarkT = "world\tfest\tmark\trcop\t3\t21\t608\t1700\tt\n";
	const std::string Mark = "world\tfest\tmark\tring27\t1\t140\t600\t80\n";
	const std::string Past = "world\tfest\tmark\ttco27\t9\t2\t300\t900\n";
	world::World Planted;
	FestLoadWorld(Planted, PeakText + Final + MarkT + Mark + Past);
	const std::string Out = FestLinesWorld(FestTextWorld(Planted), "world\tfest\t");
	const std::string Want = Race + Final + MarkT + Mark + "world\tfest\tmark\ttco27\t0\t2\t300\t900\n";
	Expect(Out == Want, "FestSaveRoundTrip: the race, final and mark lines round-trip exactly and a mark past Count reads None (" +
							std::string(Out == Want ? "as written" : "got:\n" + FestLinesWorld(Out, "world\tfest\tf") + FestLinesWorld(Out, "world\tfest\tm")) + ")");
}
} // namespace

void SegWorld()
{
	// FW-1
	if (Only("FestReignCopy"))
	{
		FestReignCopy();
	}
	// FW-2a: the race and its marks
	if (Only("FestRaceDeterministic"))
	{
		FestRaceDeterministic();
	}
	if (Only("FestRaceTiesToPerson"))
	{
		FestRaceTiesToPerson();
	}
	if (Only("FestMarksNested"))
	{
		FestMarksNested();
	}
	if (Only("FestLadderCutoff"))
	{
		FestLadderCutoff();
	}
	if (Only("FestReloadMidRace"))
	{
		FestReloadMidRace();
	}
	if (Only("FestOldSaveMidFestival"))
	{
		FestOldSaveMidFestival();
	}
	if (Only("FestSaveRoundTrip"))
	{
		FestSaveRoundTrip();
	}
}
} // namespace festivals_test
