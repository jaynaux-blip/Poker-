// festivals: the world's side of Festival Season (update 4 SPEC v1.7 §2.15.1; F-world's file): the race and its marks, the crown and
// its tickets, the roll and the defender, Discover widened, the saves' festival lines, texts and letters, the Pass's first edition on
// two rents, the agenda merge, the Rerun keys. Called from festivals_main.cpp (its kit: Expect).
// Phase A (§6 PA-48): registered, empty. G1: F-world's checks land package by package (FW-1: FestReignCopy, the strings and the
// sponsor half; FW-2a: the race and its marks: FestRaceDeterministic, FestRaceTiesToPerson, FestMarksNested, FestLadderCutoff,
// FestReloadMidRace, FestOldSaveMidFestival and FestSaveRoundTrip's race part, with the race's budget lines; FW-2b: the crown:
// FestCrownSkipInvariant, FestCrownWaitsForHero, FestNoRecrown, FestHallScoreExcludesCrown, FestAwardRebuildIgnoresCrown,
// FestSaveRoundTrip's final line and day-66 curve field, FestCalendarFacts' world half, with the crown's budget lines). Every helper here
// is in an anonymous namespace (PLAN_G1 §1 item 8: festivals_main.cpp shares the executable); each check runs alone as "festivals_test
// <Check>" (the kit's Only, FN-1). The race checks build one Blank world before RCOP 2026 opens and run it through the edition (about
// 18 ms a world day), planting races through the world text; the crown's run it on through FEIGN 2026, the first rival festival, to
// day 67, never past FEIGN I (§8.1: "three Blank worlds to day 65").
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
			// (Day 35's read comes at LastEnd itself, 03:57: the crown is the 04:00 slice's, FW-2b.)
			W.AdvanceTo(std::min(static_cast<double>(Day) * 1440.0 + 600.0, End));
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
	// The next edition on RCOP's own site (FestDay is called here at the loaded world's clock, day 24: a rival's edition such as FEIGN
	// 2026 would be skipped for its site's launch lying ahead of that clock, Ignite's Nov 2, which is FestDay's rule, not the old save's).
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Next = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Next = !Next && Rcop && F.Site == Rcop->Site && F.FirstDay > Rcop->LastDay ? &F : Next;
	}
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

// ------------------------------------------------------------------ FW-2b: the crown (§2.6.1, §2.1.4, §2.5.5, §2.13, §2.14)

/** A world day's minute. */
double FestAtWorld(int Day, int Minute = 0)
{
	return static_cast<double>(Day) * 1440.0 + static_cast<double>(Minute);
}

/** The first hour boundary at or after a world minute: the slice FestHour runs at (World::AdvanceTo's hourly block). */
double FestHourAfterWorld(double Min)
{
	return std::ceil(Min / 60.0) * 60.0;
}

/** "day 35 4:00" for a message. */
std::string FestWhenWorld(double Min)
{
	if (Min < 0.0)
	{
		return "never";
	}
	const int Day = static_cast<int>(std::floor(Min / 1440.0));
	const int Rest = static_cast<int>(std::llround(Min - FestAtWorld(Day)));
	char Buf[48];
	std::snprintf(Buf, sizeof(Buf), "day %d %d:%02d", Day, Rest / 60, Rest % 60);
	return Buf;
}

/** Text's lines that start with Prefix, counted. */
size_t FestCountLinesWorld(const std::string& Text, const std::string& Prefix)
{
	const std::string L = FestLinesWorld(Text, Prefix);
	return static_cast<size_t>(std::count(L.begin(), L.end(), '\n'));
}

/** The crown Honors a world holds for a series (EventId "<key>#pos"). */
std::vector<ss::world::Honor> FestCrownsOfWorld(const ss::world::World& W, const std::string& Key)
{
	std::vector<ss::world::Honor> Out;
	for (const ss::world::Honor& H : W.Honors())
	{
		if (H.EventId == Key + "#pos")
		{
			Out.push_back(H);
		}
	}
	return Out;
}

/** The FestivalCrown posts in a world's log naming What (the edition's name). */
std::vector<ss::world::WorldEvent> FestCrownPostsWorld(const ss::world::World& W, const std::string& What)
{
	std::vector<ss::world::WorldEvent> Out;
	for (const ss::world::WorldEvent& E : W.Events())
	{
		if (E.Kind == ss::world::EventKind::FestivalCrown && E.What == What)
		{
			Out.push_back(E);
		}
	}
	return Out;
}

/** The hero's mark record for a series (nullptr: none). */
const ss::world::HeroFestMark* FestMarkOfWorld(const ss::world::World& W, const std::string& Key)
{
	for (const ss::world::HeroFestMark& M : ss::world::HeroMarks(W))
	{
		if (M.Key == Key)
		{
			return &M;
		}
	}
	return nullptr;
}

/** §2.11's C1 filled for an edition: "{Festival} {Year} Player of the Series", {Festival} the brand's name without its year (FW-1's
 *  {Year}-beside-{Festival} convention: "RCOP" and 2026). */
std::string FestC1World(const ss::fest::Festival& F)
{
	const std::string Year = std::to_string(F.Year);
	std::string Name = F.Name;
	if (Name.size() > Year.size() + 1 && Name.compare(Name.size() - Year.size() - 1, Year.size() + 1, " " + Year) == 0)
	{
		Name.erase(Name.size() - Year.size() - 1);
	}
	std::string S = "{Festival} {Year} Player of the Series";
	S.replace(S.find("{Festival}"), 10, Name);
	S.replace(S.find("{Year}"), 6, Year);
	return S;
}

/** Two world texts compared: "" when equal, else the first differing line of each. */
std::string FestTextDiffWorld(const std::string& A, const std::string& B)
{
	if (A == B)
	{
		return std::string();
	}
	size_t K = 0;
	while (K < A.size() && K < B.size() && A[K] == B[K])
	{
		++K;
	}
	const size_t Start = A.rfind('\n', K == 0 ? 0 : K - 1);
	const size_t From = Start == std::string::npos ? 0 : Start + 1;
	auto LineAt = [From](const std::string& S) {
		const size_t End = S.find('\n', From);
		return S.substr(From, std::min<size_t>(End == std::string::npos ? S.size() - From : End - From, 160));
	};
	return "first difference at byte " + std::to_string(K) + ": '" + LineAt(A) + "' / '" + LineAt(B) + "'";
}

/** A world text with its header's Rev (the world\thead line's sixth value) replaced by '*': Rev counts the AdvanceTo targets that
 *  changed something (World::RunTurn, the views' cache key), so 30, 1 and 720 calls over the same days write 30, 1 and 720 more of it;
 *  every other byte of the world is the same however the days were stepped. */
std::string FestNoRevWorld(std::string Text)
{
	if (Text.compare(0, 11, "world\thead\t") != 0)
	{
		return Text;
	}
	size_t At = 0;
	for (int Tab = 0; Tab < 7 && At != std::string::npos; ++Tab)
	{
		At = Text.find('\t', At + 1);
	}
	const size_t End = At == std::string::npos ? At : Text.find_first_of("\t\n", At + 1);
	if (At != std::string::npos && End != std::string::npos)
	{
		Text.replace(At + 1, End - At - 1, "*");
	}
	return Text;
}

/** RCOP 2026's crown three ways from its day 20, 12:00 (FestMidWorld) to day 50, 12:00: 30 one-day steps, one 30-day step and 720
 *  one-hour steps, each AdvanceTo a target of its own. The hourly run records the first step that read the crown, and whether the
 *  step before it still read the race live and uncrowned. Built once. */
struct FestSkipRunWorld
{
	std::unique_ptr<ss::world::World> Days, Skip, Hours;
	double Crown = -1.0;     // the hourly run's first step reading RCOP 2026 crowned (its clock)
	bool LiveBefore = false; // the step before: RCOP's race live, uncrowned
	double Ms[3] = {0.0, 0.0, 0.0};
};

FestSkipRunWorld& FestSkipWorld()
{
	namespace world = ss::world;
	static FestSkipRunWorld Run;
	if (!Run.Days)
	{
		const world::World& Mid = FestMidWorld();
		const double From = Mid.Clock();
		auto T0 = std::chrono::steady_clock::now();
		Run.Days = std::make_unique<world::World>(Mid);
		for (int K = 1; K <= 30; ++K)
		{
			Run.Days->AdvanceTo(From + static_cast<double>(K) * 1440.0);
		}
		auto T1 = std::chrono::steady_clock::now();
		Run.Ms[0] = std::chrono::duration<double, std::milli>(T1 - T0).count();
		Run.Skip = std::make_unique<world::World>(Mid);
		Run.Skip->AdvanceTo(From + 30.0 * 1440.0);
		T0 = std::chrono::steady_clock::now();
		Run.Ms[1] = std::chrono::duration<double, std::milli>(T0 - T1).count();
		Run.Hours = std::make_unique<world::World>(Mid);
		bool Live = false;
		for (int K = 1; K <= 720; ++K)
		{
			Run.Hours->AdvanceTo(From + static_cast<double>(K) * 60.0);
			const bool Crowned = world::FestCrownedAt(*Run.Hours, "rcop") >= 0.0;
			if (Crowned && Run.Crown < 0.0)
			{
				Run.Crown = Run.Hours->Clock();
				Run.LiveBefore = Live;
			}
			Live = !Crowned && world::FestRaceKey(*Run.Hours) == "rcop";
		}
		Run.Ms[2] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	}
	return Run;
}

/** RCOP 2026 just after its crown: the peak world (LastEnd, 03:57 on day 35) run two hours past the 04:00 slice. Built once. */
const ss::world::World& FestAfterWorld()
{
	static std::unique_ptr<ss::world::World> After;
	if (!After)
	{
		const ss::fest::Festival* Rcop = FestRcopWorld();
		After = std::make_unique<ss::world::World>(*FestPeakWorld().W);
		After->AdvanceTo(FestHourAfterWorld(Rcop ? ss::fest::LastEnd(*Rcop) : FestAtWorld(35)) + 120.0);
	}
	return *After;
}

/** FEIGN 2026, the first rival festival (Ignite, Nov 30 - Dec 9; its Main on day 65 at 18:00), from the skip run's day 50: the hero
 *  registers for its Main at the start and is still seated six hours past fest::LastEnd, wins it there (the table report naming the
 *  race's leading person second), and the world runs on to day 67. Built once. */
struct FestFeignRunWorld
{
	std::unique_ptr<ss::world::World> W;
	const ss::fest::Festival* F = nullptr;
	ss::net::EventInstance Main;
	bool Found = false;    // FEIGN 2026 and its Main instance
	bool Opened = false;   // its race live at the Main's start
	bool HeldOpen = false; // at Held: no crown, the race open and written, PhaseOf Closing
	double End = 0.0;      // fest::LastEnd
	double Held = 0.0;     // the first hour slice after LastEnd + 6 hours: the hero finishes here
	double Crown = -1.0;   // FestCrownedAt after HeroFinished
	int Second = -1;       // the person the hero's table report names second
	std::string HeldText;  // the world text at Held
};

FestFeignRunWorld& FestFeignWorld()
{
	namespace world = ss::world;
	static FestFeignRunWorld Run;
	if (!Run.W)
	{
		Run.W = std::make_unique<world::World>(*FestSkipWorld().Skip);
		world::World& W = *Run.W;
		Run.F = ss::fest::FestivalOf("ignite.feign26");
		if (Run.F)
		{
			const ss::net::Network& N = ss::net::Site(Run.F->Site);
			for (const ss::net::EventInstance& E : N.Window(FestAtWorld(Run.F->MainDay), FestAtWorld(Run.F->MainDay + 1)))
			{
				if (!Run.Found && N.TemplateOf(E).Id == Run.F->MainId)
				{
					Run.Main = E;
					Run.Found = true;
				}
			}
		}
		if (Run.Found)
		{
			const std::string& Key = Run.F->SeriesId;
			Run.End = ss::fest::LastEnd(*Run.F);
			W.AdvanceTo(Run.Main.Start);
			Run.Opened = world::FestRaceKey(W) == Key && world::FestCrownedAt(W, Key) < 0.0;
			W.HeroEntered(Run.Main.Id, std::string());
			Run.Held = FestHourAfterWorld(Run.End) + 360.0;
			W.AdvanceTo(Run.Held);
			Run.HeldText = FestTextWorld(W);
			Run.HeldOpen = world::FestCrownedAt(W, Key) < 0.0 && world::FestRaceKey(W) == Key && FestCountLinesWorld(Run.HeldText, "world\tfest\trace\t" + Key + "\t") == 1 &&
						   ss::fest::PhaseOf(*Run.F, Run.Held, &W) == ss::fest::Phase::Closing;
			// The hero wins it heads-up against the race's leading person (an Ignite regular), whom the table report names second:
			// a final-table finish in a Main-curve Main, so a world\trecent record with the curve field (FestSaveRoundTrip, R-25).
			world::World::TableReport Report;
			for (const auto& Row : world::FestTop(W, 10))
			{
				if (Row.first >= 0 && Run.Second < 0)
				{
					Run.Second = Row.first;
					Report.Places.emplace_back(Row.first, 2);
				}
			}
			W.HeroFinished(Run.Main.Id, 1, Run.Main.Pool / 4, Report);
			W.AdvanceTo(Run.Held + 120.0);
			Run.Crown = world::FestCrownedAt(W, Key);
			W.AdvanceTo(FestAtWorld(67));
		}
	}
	return Run;
}

/** The crown's budget lines (§2.14; printed, asserted only under SS_PERF=1 as the median of five): FestHour an hour over 720 hours
 *  of RCOP 2026's race from its day 20 (before LastEnd a compare; from LastEnd on the queue's scan, here never ending: this world's
 *  queue still holds the edition's day-20 events), and the crown itself (the sort of 610 scorers and its writes) on copies of the peak. */
void FestCrownBudgetsWorld(const FestSkipRunWorld& Skip)
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		return;
	}
	const double End = ss::fest::LastEnd(*Rcop);
	std::vector<double> Compare, Scan, Crown;
	for (int Round = 0; Round < 5; ++Round)
	{
		world::World D = FestMidWorld();
		const double From = D.Clock();
		double Before = 0.0, After = 0.0;
		int NBefore = 0, NAfter = 0;
		for (int K = 1; K <= 720; ++K)
		{
			const double Hour = FestHourAfterWorld(From) + static_cast<double>(K - 1) * 60.0;
			const auto T0 = std::chrono::steady_clock::now();
			world::FestHour(D, Hour);
			const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
			(Hour < End ? Before : After) += Ms;
			++(Hour < End ? NBefore : NAfter);
		}
		Compare.push_back(NBefore ? Before / NBefore : 0.0);
		Scan.push_back(NAfter ? After / NAfter : 0.0);
		world::World P = *FestPeakWorld().W;
		const auto T0 = std::chrono::steady_clock::now();
		world::FestHour(P, FestHourAfterWorld(End));
		Crown.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count());
	}
	auto Median = [](std::vector<double> V) {
		std::sort(V.begin(), V.end());
		return V[V.size() / 2];
	};
	std::printf("  crown (RCOP 2026): FestHour %.5f ms an hour before LastEnd (a compare), %.5f ms from it (the queue's scan; budget 0.05 ms); "
				"the crown %.3f ms (%zu scorers sorted; budget 1 ms); medians of 5; the three runs day 20 -> 50 (1 day x 30, 30 days, 1 hour x 720): %.0f / %.0f / %.0f ms\n",
		Median(Compare), Median(Scan), Median(Crown), world::FestTop(*FestPeakWorld().W, kFestAllWorld).size(), Skip.Ms[0], Skip.Ms[1], Skip.Ms[2]);
	if (std::getenv("SS_PERF"))
	{
		Expect(Median(Compare) <= 0.05 && Median(Scan) <= 0.05, "FestHour costs at most 0.05 ms an hour (" + std::to_string(Median(Compare)) + " and " + std::to_string(Median(Scan)) + " ms)");
		Expect(Median(Crown) < 1.0, "the crown costs under 1 ms (" + std::to_string(Median(Crown)) + " ms)");
	}
}

/**
 * FestCrownSkipInvariant (§2.15.1, §2.6.1, §2.12; V1): 30 one-day steps, one 30-day step and 720 one-hour steps across RCOP 2026's
 * crown (day 20, 12:00 to day 50, 12:00) give byte-identical world text (the header's Rev aside: it counts the calls, FestNoRevWorld),
 * and the crown lands in the first hour slice at or after
 * fest::LastEnd with no festival Pending left (here 04:00 on day 35: the hourly run read the race live at 03:00). The crown's writes: one
 * Honor (C1's title, the series id + "#pos", the points in Entries, Prize 0, At the crown minute), one FestivalCrown post (the winner,
 * the edition's name, their cashes and points; RiverLine's bits: none; no FlagHero), +6 online fame to the person who won, the hero's
 * mark (their rank, MarkFor's grade, no ticket below FINALIST; FestStanding reads it outside the top ten), FestLast with the top ten in
 * rank order, the race closed; S = 0 closes a race with no champion (FestCalendarFacts' world half).
 * Planted bug: the crown moved into NewDay.
 */
void FestCrownSkipInvariant()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	Expect(Rcop != nullptr, "FestCrownSkipInvariant: RCOP 2026 is in fest::All()");
	if (!Rcop)
	{
		return;
	}
	FestSkipRunWorld& Run = FestSkipWorld();
	const std::string TextD = FestNoRevWorld(FestTextWorld(*Run.Days));
	const std::string TextS = FestNoRevWorld(FestTextWorld(*Run.Skip));
	const std::string TextH = FestNoRevWorld(FestTextWorld(*Run.Hours));
	const std::string DiffDS = FestTextDiffWorld(TextD, TextS);
	const std::string DiffHS = FestTextDiffWorld(TextH, TextS);
	Expect(TextS.find("\t*\t") != std::string::npos && DiffDS.empty() && DiffHS.empty(),
		"FestCrownSkipInvariant: 30 x 1 day, 1 x 30 days and 720 x 1 hour across RCOP 2026's crown give byte-identical world text, the header's Rev aside (" +
			std::to_string(TextS.size()) + " B" + (DiffDS.empty() ? "" : "; days: " + DiffDS) + (DiffHS.empty() ? "" : "; hours: " + DiffHS) + ")");

	const double End = ss::fest::LastEnd(*Rcop);
	const double Want = FestHourAfterWorld(End);
	const double AtD = world::FestCrownedAt(*Run.Days, "rcop");
	const double AtS = world::FestCrownedAt(*Run.Skip, "rcop");
	Expect(Run.Crown == Want && Run.LiveBefore && AtD == Want && AtS == Want,
		"FestCrownSkipInvariant: the crown lands in the first slice after LastEnd (" + FestWhenWorld(End) + "): " + FestWhenWorld(Want) + " (the hourly run read it at " +
			FestWhenWorld(Run.Crown) + (Run.LiveBefore ? ", the race live the hour before" : ", the race not live the hour before") + "; the day and skip runs " +
			FestWhenWorld(AtD) + ", " + FestWhenWorld(AtS) + ")");

	// The crown's writes, on the skip run (a person wins RCOP 2026: the hero's one day-14 cash is far down the race).
	const world::World& S = *Run.Skip;
	const std::vector<world::Honor> Crowns = FestCrownsOfWorld(S, "rcop");
	const std::vector<world::WorldEvent> Posts = FestCrownPostsWorld(S, Rcop->Name);
	const world::World& Peak = *FestPeakWorld().W;
	const auto Order = world::FestTop(Peak, kFestAllWorld); // the race at LastEnd, ranked: what the crown ranks
	bool Writes = Crowns.size() == 1 && Posts.size() == 1 && !Order.empty();
	std::string Got = std::to_string(Crowns.size()) + " Honor(s), " + std::to_string(Posts.size()) + " post(s)";
	if (Writes)
	{
		const world::Honor& H = Crowns.front();
		const world::WorldEvent& E = Posts.front();
		const int Winner = Order.front().first;
		const int Points = static_cast<int>(std::llround(Order.front().second.Points));
		const world::Npc* P = Peak.Get(Winner);
		const size_t Online = static_cast<size_t>(ss::world::Rep::Online);
		const double FameBefore = P ? static_cast<double>(P->Fame[Online]) : 0.0;
		const world::World After = FestAfterWorld();
		const world::Npc* Q = After.Get(Winner);
		const double FameAfter = Q ? static_cast<double>(Q->Fame[Online]) : 0.0;
		const auto Top = world::FestTop(After, kFestAllWorld);
		const std::vector<std::pair<int, world::FestScore>> Ten(Order.begin(), Order.begin() + std::min<std::ptrdiff_t>(10, static_cast<std::ptrdiff_t>(Order.size())));
		Writes = Winner >= 0 && P && H.Title == FestC1World(*Rcop) && H.Title == Rcop->Name + " Player of the Series" && H.Npc == Winner && H.Name == P->Name &&
				 H.Year == 2026 && H.Prize == 0 && H.Entries == Points && H.At == Want && E.Npc == Winner && E.At == Want && E.Value == Points &&
				 E.Amount == Order.front().second.Cashes && (E.Flags & ss::world::FlagSiteBits) == 0 && (E.Flags & ss::world::FlagHero) == 0 &&
				 std::fabs(FameAfter - FameBefore - 6.0) < 1e-3 && FestSameRaceWorld(Top, Ten) && FestCountLinesWorld(FestTextWorld(After), "world\tfest\trace\t") == 0;
		Got += "; '" + H.Title + "' to " + H.Name + " (npc " + std::to_string(Winner) + "), " + std::to_string(H.Entries) + " pts of " + std::to_string(Points) + ", " +
			   std::to_string(E.Amount) + " cashes, fame " + std::to_string(FameBefore) + " -> " + std::to_string(FameAfter) + ", the last crown's top " + std::to_string(Top.size());
		// The hero's mark (their one day-14 cash): their rank in the race, MarkFor's grade, no ticket below FINALIST; outside the top ten
		// the standing reads it through the Afterglow.
		int HeroRank = 0;
		for (size_t K = 0; K < Order.size(); ++K)
		{
			HeroRank = Order[K].first == -1 ? static_cast<int>(K) + 1 : HeroRank;
		}
		const world::HeroFestMark* M = FestMarkOfWorld(After, "rcop");
		const int Sc = static_cast<int>(Order.size());
		Writes = Writes && HeroRank > 10 && M && M->Rank == HeroRank && M->Scorers == Sc && M->Mark == world::MarkFor(HeroRank, Sc) &&
				 M->Points == Order[static_cast<size_t>(HeroRank - 1)].second.Points && M->Ticket == (M->Mark >= world::FestMark::Finalist) &&
				 world::HeroMark(After, "rcop") == M->Mark && world::FestStanding(After, -1) == std::make_pair(HeroRank, Sc) && world::HeroMarks(After).size() == 1;
		Got += "; the hero's mark " + std::to_string(M ? static_cast<int>(M->Mark) : -1) + " at rank " + std::to_string(HeroRank) + " of " + std::to_string(Sc);
	}
	std::printf("  FestCrownSkipInvariant: RCOP 2026 crowned at %s (LastEnd %s): %s\n", FestWhenWorld(Want).c_str(), FestWhenWorld(End).c_str(), Got.c_str());
	Expect(Writes, "FestCrownSkipInvariant: the crown writes one Honor (C1's title, '<series>#pos', the points, Prize 0), one FestivalCrown post (RiverLine's: no site bits, no FlagHero), "
				   "+6 online fame to the person who won, the hero's mark (MarkFor, no ticket below FINALIST), FestLast's top ten, and closes the race (" + Got + ")");
	FestCrownBudgetsWorld(Run);
}

/**
 * FestCrownWaitsForHero (§2.15.1, §2.6.1 condition 3): on FEIGN 2026's Main (Ignite, day 65; RCOP's Main is locked until G3), the hero
 * still seated six hours past fest::LastEnd holds the crown (no crown Honor, the race open and written, PhaseOf Closing) until
 * World::HeroFinished, and it lands in the next hour slice. The rival's crown: one Honor, one post carrying Ignite's site bits (so it
 * shows on Ignite's News and stays off RiverLine's), FlagHero exactly when the hero won; the hero's mark is MarkFor(rank, scorers) with
 * its crown ticket from FINALIST up, and the views read the last crown through the Afterglow.
 * Planted bug: the Queue condition dropped.
 */
void FestCrownWaitsForHero()
{
	namespace world = ss::world;
	FestFeignRunWorld& Run = FestFeignWorld();
	Expect(Run.Found && Run.Opened, "FestCrownWaitsForHero: FEIGN 2026's Main is on its day 10 (day 65) and its race is live at the Main's start in a Blank world (Ignite open since Nov 2)");
	if (!Run.Found)
	{
		return;
	}
	const std::string& Key = Run.F->SeriesId;
	Expect(Run.HeldOpen, "FestCrownWaitsForHero: the hero still seated in FEIGN 2026's Main six hours past LastEnd (" + FestWhenWorld(Run.End) + ") holds the crown: none at " +
							 FestWhenWorld(Run.Held) + ", the race open and written, Closing (crowned at " + FestWhenWorld(Run.Crown) + ")");
	Expect(Run.Crown == Run.Held + 60.0, "FestCrownWaitsForHero: after HeroFinished the crown lands in the next hour slice (" + FestWhenWorld(Run.Crown) + "; want " +
											 FestWhenWorld(Run.Held + 60.0) + ")");

	const world::World& W = *Run.W;
	const std::vector<world::Honor> Crowns = FestCrownsOfWorld(W, Key);
	const std::vector<world::WorldEvent> Posts = FestCrownPostsWorld(W, Run.F->Name);
	const auto Top = world::FestTop(W, kFestAllWorld);
	const std::pair<int, int> Hero = world::FestStanding(W, -1);
	const world::HeroFestMark* M = FestMarkOfWorld(W, Key);
	bool Rival = Crowns.size() == 1 && Posts.size() == 1 && !Top.empty();
	std::string Got = std::to_string(Crowns.size()) + " Honor(s), " + std::to_string(Posts.size()) + " post(s)";
	if (Rival)
	{
		const world::WorldEvent& E = Posts.front();
		const bool HeroWon = Top.front().first == -1;
		Rival = Crowns.front().Npc == Top.front().first && Crowns.front().Title == FestC1World(*Run.F) && Crowns.front().At == Run.Crown && E.Npc == Top.front().first &&
				ss::world::FlagSiteOf(E.Flags) == ss::net::SiteId::Ignite && ((E.Flags & ss::world::FlagHero) != 0) == HeroWon && world::FestRaceKey(W) == Key;
		Got += "; '" + Crowns.front().Title + "' to " + Crowns.front().Name + ", the post's site " + std::to_string(static_cast<int>(ss::world::FlagSiteOf(E.Flags))) +
			   (HeroWon ? ", the hero's" : "");
	}
	Expect(Rival, "FestCrownWaitsForHero: FEIGN 2026's crown is one Honor (C1's title) and one post with Ignite's site bits, FlagHero only for the hero's (" + Got + ")");
	const bool Marked = M && Hero.first > 0 && M->Rank == Hero.first && M->Scorers == Hero.second && M->Scorers == static_cast<int>(world::FestStanding(W, Top.front().first).second) &&
						M->Mark == world::MarkFor(M->Rank, M->Scorers) && world::HeroMark(W, Key) == M->Mark && M->Ticket == (ss::fest::kCrownTickets && M->Mark >= world::FestMark::Finalist) &&
						world::HeroMark(W, "ignite.feign27") == world::FestMark::None;
	std::printf("  FestCrownWaitsForHero: FEIGN 2026's LastEnd %s, the hero seated to %s, crowned %s (%s); the hero's mark %d, rank %d of %d%s\n", FestWhenWorld(Run.End).c_str(),
		FestWhenWorld(Run.Held).c_str(), FestWhenWorld(Run.Crown).c_str(), Got.c_str(), M ? static_cast<int>(M->Mark) : -1, M ? M->Rank : 0, M ? M->Scorers : 0, M && M->Ticket ? ", a crown ticket" : "");
	Expect(Marked, "FestCrownWaitsForHero: the hero's mark is MarkFor(rank, scorers) with its crown ticket from FINALIST up (rank " + std::to_string(M ? M->Rank : 0) + " of " +
					   std::to_string(M ? M->Scorers : 0) + ", mark " + std::to_string(M ? static_cast<int>(M->Mark) : -1) + (M && M->Ticket ? ", a ticket" : "") + "; standing " +
					   std::to_string(Hero.first) + " of " + std::to_string(Hero.second) + ")");
}

/**
 * FestNoRecrown (§2.15.1, §2.6.1 "idempotent", §2.5.2 rule 1): a save just after RCOP 2026's crown reloads with its last crown and the
 * hero's mark, and advances to day 40 as the unbroken run (the same text: one crown Honor, the Afterglow's FestLast gone after three
 * days); FestDay on RCOP's first day again, with a planted RCOP finish, reopens nothing (no race line) and nothing crowns twice; a stale
 * race still open when the next edition opens is crowned first (§2.5.2 rule 1).
 * Planted bugs: FestDay reopening a crowned key; the stale race closed uncrowned.
 */
void FestNoRecrown()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		Expect(false, "FestNoRecrown: RCOP 2026 is in fest::All()");
		return;
	}
	world::World Run = FestAfterWorld();
	const std::string Saved = FestTextWorld(Run);
	world::World Back;
	FestLoadWorld(Back, Saved);
	const bool Loaded = FestTextWorld(Back) == Saved && world::FestCrownedAt(Back, "rcop") == world::FestCrownedAt(Run, "rcop") && world::FestRaceKey(Back) == "rcop" &&
						FestSameRaceWorld(world::FestTop(Back, kFestAllWorld), world::FestTop(Run, kFestAllWorld)) && world::HeroMarks(Back).size() == world::HeroMarks(Run).size();
	Run.AdvanceTo(FestAtWorld(40));
	Back.AdvanceTo(FestAtWorld(40));
	const std::string RunText = FestTextWorld(Run);
	const std::string BackText = FestTextWorld(Back);
	Expect(Loaded && RunText == BackText && FestCrownsOfWorld(Back, "rcop").size() == 1 && FestCountLinesWorld(BackText, "world\tfest\tfinal\t") == 0 &&
			   FestCountLinesWorld(BackText, "world\tfest\trace\t") == 0 && world::FestRaceKey(Back).empty(),
		"FestNoRecrown: a save after RCOP 2026's crown reloads with it (" + std::string(Loaded ? "as saved" : "not as saved") + ") and runs to day 40 as the unbroken run (" +
			(RunText == BackText ? "the same text" : FestTextDiffWorld(RunText, BackText)) + "): " + std::to_string(FestCrownsOfWorld(Back, "rcop").size()) +
			" crown Honor(s), the last crown gone after the Afterglow");

	world::World Again;
	FestLoadWorld(Again, Saved);
	world::FestDay(Again, Rcop->FirstDay);
	world::Pending P; // an RCOP numbered event's win, planted
	P.Online = true;
	P.Series = Rcop->SeriesId;
	P.Entries = 300;
	P.BuyIn = 2200;
	P.Format = static_cast<int>(ss::net::Format::Freezeout);
	world::FestScoreResult(Again, P, 0, 1);
	const size_t Reopened = FestCountLinesWorld(FestTextWorld(Again), "world\tfest\trace\t");
	Again.AdvanceTo(Again.Clock() + 120.0);
	Expect(Reopened == 0 && FestCrownsOfWorld(Again, "rcop").size() == 1 && FestCountLinesWorld(FestTextWorld(Again), "world\tfest\trace\t") == 0,
		"FestNoRecrown: FestDay on RCOP 2026's first day again reopens nothing (" + std::to_string(Reopened) + " race line(s) after a planted finish) and two hours on there is still one crown Honor (" +
			std::to_string(FestCrownsOfWorld(Again, "rcop").size()) + ")");

	// §2.5.2 rule 1's stale key (the calendar never leaves one): RCOP 2026's race still open (the peak world, before its crown slice)
	// when RiverLine's next edition opens is crowned first, at that minute, and then the new race opens.
	const ss::fest::Festival* Next = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Next = !Next && F.Site == Rcop->Site && F.FirstDay > Rcop->LastDay ? &F : Next;
	}
	world::World Stale;
	FestLoadWorld(Stale, FestTextWorld(*FestPeakWorld().W));
	if (Next)
	{
		world::FestDay(Stale, Next->FirstDay);
	}
	const std::vector<world::Honor> StaleCrowns = FestCrownsOfWorld(Stale, "rcop");
	Expect(Next && StaleCrowns.size() == 1 && StaleCrowns.front().At == Stale.Clock() && world::FestRaceKey(Stale) == Next->SeriesId && world::FestTop(Stale, 1).empty(),
		"FestNoRecrown: a stale race (RCOP 2026's, still open when " + std::string(Next ? Next->Name : "the next edition") + " opens) is crowned first, then the new race opens (" +
			std::to_string(StaleCrowns.size()) + " crown Honor(s); race '" + world::FestRaceKey(Stale) + "')");
}

/**
 * FestHallScoreExcludesCrown (§2.15.1, §2.6.1's two readers): the hero wins RCOP 2026's race (planted through the world text on the peak
 * world) and the crown leaves World::HallScore unchanged. The hero's crown: the Honor as the hero's (Npc -1, the world's hero name), the
 * post flagged FlagHero, the mark THE BELT with its crown ticket, FestLast's top reading the hero first.
 * Planted bug: the IsCrownHonor skip removed.
 */
void FestHallScoreExcludesCrown()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		Expect(false, "FestHallScoreExcludesCrown: RCOP 2026 is in fest::All()");
		return;
	}
	world::World W;
	FestLoadWorld(W, FestTextWorld(*FestPeakWorld().W) + "world\tfest\trace\trcop\t-1:99999:5,12:100:1\n");
	const double Before = W.HallScore(-1, 2026);
	W.AdvanceTo(FestHourAfterWorld(ss::fest::LastEnd(*Rcop)) + 30.0);
	const double After = W.HallScore(-1, 2026);
	const std::vector<world::Honor> Crowns = FestCrownsOfWorld(W, "rcop");
	const std::vector<world::WorldEvent> Posts = FestCrownPostsWorld(W, Rcop->Name);
	const world::HeroFestMark* M = FestMarkOfWorld(W, "rcop");
	const auto Top = world::FestTop(W, kFestAllWorld);
	const bool HeroCrown = Crowns.size() == 1 && Crowns.front().Npc == -1 && Crowns.front().Name == W.HeroName && Crowns.front().Entries == 99999 && Posts.size() == 1 &&
						   Posts.front().Npc == -1 && (Posts.front().Flags & ss::world::FlagHero) != 0 && Posts.front().Amount == 5 && M && M->Mark == world::FestMark::Title &&
						   M->Rank == 1 && M->Scorers == 2 && M->Points == 99999.0 && M->Ticket == ss::fest::kCrownTickets && Top.size() == 2 && Top.front().first == -1;
	Expect(HeroCrown, "FestHallScoreExcludesCrown: the hero who leads RCOP 2026's planted race is crowned: the Honor and the post the hero's (FlagHero), THE BELT with its ticket (" +
						  std::to_string(Crowns.size()) + " Honor(s), mark " + std::to_string(M ? static_cast<int>(M->Mark) : -1) + ")");
	Expect(HeroCrown && Before == After, "FestHallScoreExcludesCrown: the hero's crown Honor leaves HallScore unchanged (" + std::to_string(Before) + " -> " + std::to_string(After) + ")");
}

/**
 * FestAwardRebuildIgnoresCrown (§2.15.1, §2.6.1's two readers): an old save's trophy case rebuilt from history (World::Finish: a person
 * whose bracelets outnumber their awards) makes no bracelet of "The Championship Online 2027 Player of the Series" (a planted crown
 * Honor, first in history, though its title starts "The Championship ").
 * Planted bug: the skip removed.
 */
void FestAwardRebuildIgnoresCrown()
{
	namespace world = ss::world;
	// An Established world (a Blank one has no bracelets yet) on RCOP 2026's last day.
	const auto T0 = std::chrono::steady_clock::now();
	world::World Peak;
	Peak.Create(kFestSeedWorld, FestAtWorld(34, 600), world::Start::Established);
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	int Id = -1;
	for (int K = 0; Id < 0 && K < static_cast<int>(Peak.People().size()); ++K)
	{
		const world::Npc* P = Peak.Get(K);
		Id = P && P->Bracelets >= 1 && static_cast<int>(P->Awards.size()) == P->Bracelets + P->Rings ? K : -1;
	}
	std::printf("  FestAwardRebuildIgnoresCrown: an Established world made in %.0f ms; npc %d holds a bracelet\n", Ms, Id);
	Expect(Id >= 0, "FestAwardRebuildIgnoresCrown: the Established world holds a person with a bracelet and their awards");
	if (Id < 0)
	{
		return;
	}
	const world::Npc& P = *Peak.Get(Id);
	const std::string Title = "The Championship Online 2027 Player of the Series";
	const std::string Planted = "world\thonor\t2027\t" + Title + "\ttco27#pos\t" + std::to_string(Id) + "\t" + P.Name + "\t0\t4200\t" + std::to_string(FestAtWorld(34) + 240.0) + "\n";
	std::string Text = FestLinesWorld(FestTextWorld(Peak), "world\tawards\t" + std::to_string(Id) + "\t", false);
	const size_t First = Text.find("world\thonor\t");
	Text.insert(First == std::string::npos ? Text.size() : First, Planted);
	world::World Old;
	FestLoadWorld(Old, Text);
	const world::Npc* Q = Old.Get(Id);
	bool Clean = Q && static_cast<int>(Q->Awards.size()) == P.Bracelets + P.Rings && !Old.Honors().empty() && world::IsCrownHonor(Old.Honors().front());
	for (const world::Award& A : Q ? Q->Awards : std::vector<world::Award>())
	{
		Clean = Clean && A.Event.find("Player of the Series") == std::string::npos;
	}
	Expect(Clean, "FestAwardRebuildIgnoresCrown: '" + Title + "' makes no bracelet in the old-save rebuild (npc " + std::to_string(Id) + ", " + std::to_string(P.Bracelets) +
					  " bracelet(s): " + std::to_string(Q ? Q->Awards.size() : 0) + " award(s) rebuilt" +
					  (Q && !Q->Awards.empty() && !Q->Awards.front().Event.empty() ? ", the first '" + Q->Awards.front().Event + "'" : "") + ")");
}

/** PhaseOf at each minute, against the phase wanted; "" when all match, else the first mismatch. */
std::string FestPhasesWorld(const ss::fest::Festival& F, const ss::world::World& W, const std::vector<std::pair<double, ss::fest::Phase>>& Steps)
{
	static const char* const Names[] = {"Off", "Announced", "Buildup", "Live", "MainDay", "Closing", "Afterglow"};
	for (const auto& S : Steps)
	{
		const ss::fest::Phase Got = ss::fest::PhaseOf(F, S.first, &W);
		if (Got != S.second)
		{
			return F.Name + " at " + FestWhenWorld(S.first) + ": " + Names[static_cast<int>(Got)] + " (want " + Names[static_cast<int>(S.second)] + ")";
		}
	}
	return std::string();
}

/**
 * FestCalendarFacts, F-world's world half (§2.15.1, §2.1.4, R-13): with a world, the Closing runs to the crown and the Afterglow three
 * days from the crown minute: RCOP 2026's (04:00 on day 35, three minutes after LastEnd) and FEIGN 2026's (held hours past LastEnd by
 * the seated hero). An edition never crowned reads Closing to LastEnd + 2 days, then Off, and never the Afterglow: an old save's RCOP (no
 * race this edition) and a race with no scorer (the crown closes it with no champion: no Honor, no post, no FestLast, no mark).
 * Planted bug: the Afterglow counted from LastEnd (fest::PhaseOf).
 */
void FestCalendarFactsWorld()
{
	namespace world = ss::world;
	using ss::fest::Phase;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		Expect(false, "FestCalendarFacts (world): RCOP 2026 is in fest::All()");
		return;
	}
	const double Day = 1440.0;
	const double End = ss::fest::LastEnd(*Rcop);
	const world::World& S = *FestSkipWorld().Skip;
	const double C = world::FestCrownedAt(S, "rcop");
	const std::string BadRcop = FestPhasesWorld(*Rcop, S,
		{{FestAtWorld(Rcop->LastDay + 1) - 1.0, Phase::MainDay}, {FestAtWorld(Rcop->LastDay + 1), Phase::Closing}, {End, Phase::Closing}, {C - 1.0, Phase::Closing},
			{C, Phase::Afterglow}, {C + 3.0 * Day - 1.0, Phase::Afterglow}, {C + 3.0 * Day, Phase::Off}});
	Expect(C > End && BadRcop.empty(), "FestCalendarFacts (world): RCOP 2026's Closing runs to the crown minute (" + FestWhenWorld(C) + ", LastEnd " + FestWhenWorld(End) +
										   ") and the Afterglow three days from it" + (BadRcop.empty() ? "" : ": " + BadRcop));

	FestFeignRunWorld& Feign = FestFeignWorld();
	std::string BadFeign = Feign.Found ? std::string() : std::string("FEIGN 2026's Main not found");
	if (Feign.Found)
	{
		const double Cf = Feign.Crown;
		BadFeign = Cf < Feign.End + 60.0 ? "crowned at " + FestWhenWorld(Cf) : FestPhasesWorld(*Feign.F, *Feign.W,
			{{Feign.End, Phase::Closing}, {Cf - 1.0, Phase::Closing}, {Cf, Phase::Afterglow}, {Cf + 3.0 * Day - 1.0, Phase::Afterglow}, {Cf + 3.0 * Day, Phase::Off}});
	}
	Expect(BadFeign.empty(), "FestCalendarFacts (world): FEIGN 2026's Closing holds past LastEnd to its late crown (" + FestWhenWorld(Feign.Crown) + ") and its Afterglow runs three days from it" +
								 (BadFeign.empty() ? "" : ": " + BadFeign));

	// Never crowned (R-13): an old save's RCOP (no race this edition), and a race with no scorer.
	const std::string Peak = FestTextWorld(*FestPeakWorld().W);
	world::World Old;
	FestLoadWorld(Old, FestLinesWorld(Peak, "world\tfest\t", false));
	Old.AdvanceTo(End + 2.0 * Day + 60.0);
	world::World Empty;
	FestLoadWorld(Empty, Peak + "world\tfest\trace\trcop\t\n");
	Empty.AdvanceTo(FestHourAfterWorld(End) + 60.0);
	const bool NoChampion = world::FestCrownedAt(Empty, "rcop") < 0.0 && FestCrownPostsWorld(Empty, Rcop->Name).empty() && world::HeroMarks(Empty).empty() &&
							FestLinesWorld(FestTextWorld(Empty), "world\tfest\t").empty() && world::FestRaceKey(Empty).empty();
	std::string BadNever;
	for (const world::World* W : {&Old, &Empty})
	{
		std::vector<std::pair<double, Phase>> Steps = {{End, Phase::Closing}, {End + 2.0 * Day - 1.0, Phase::Closing}, {End + 2.0 * Day, Phase::Off}};
		for (double M = FestAtWorld(Rcop->LastDay + 1); M < End + 6.0 * Day; M += 60.0)
		{
			Steps.emplace_back(M, M < End + 2.0 * Day ? Phase::Closing : Phase::Off);
		}
		const std::string Bad = world::FestCrownedAt(*W, "rcop") >= 0.0 ? std::string("crowned") : FestPhasesWorld(*Rcop, *W, Steps);
		BadNever += Bad.empty() || !BadNever.empty() ? std::string() : (W == &Old ? "the old save: " : "no scorer: ") + Bad;
	}
	Expect(NoChampion && BadNever.empty(), "FestCalendarFacts (world): an edition never crowned (an old save's RCOP; a race with no scorer, closed with no champion" +
											   std::string(NoChampion ? "" : ": NOT clean") + ") reads Closing to LastEnd + 2 days, then Off, never the Afterglow" +
											   (BadNever.empty() ? "" : ": " + BadNever));
}

/** A world's world\trecent records (each split on '~'), every person's. */
std::vector<std::vector<std::string>> FestRecentWorld(const std::string& Text)
{
	std::vector<std::vector<std::string>> Out;
	const std::string Lines = FestLinesWorld(Text, "world\trecent\t");
	size_t Start = 0;
	while (Start < Lines.size())
	{
		const size_t End = Lines.find('\n', Start);
		const std::string Line = Lines.substr(Start, End - Start);
		const size_t Tab = Line.find('\t', std::string("world\trecent\t").size());
		std::string Body = Tab == std::string::npos ? std::string() : Line.substr(Tab + 1);
		size_t From = 0;
		while (From <= Body.size() && !Body.empty())
		{
			size_t Bar = Body.find('|', From);
			Bar = Bar == std::string::npos ? Body.size() : Bar;
			std::vector<std::string> Fields;
			const std::string Rec = Body.substr(From, Bar - From);
			size_t F0 = 0;
			for (size_t K = 0; K <= Rec.size(); ++K)
			{
				if (K == Rec.size() || Rec[K] == '~')
				{
					Fields.push_back(Rec.substr(F0, K - F0));
					F0 = K + 1;
				}
			}
			Out.push_back(Fields);
			From = Bar + 1;
		}
		Start = End + 1;
	}
	return Out;
}

/**
 * FestSaveRoundTrip, the crown's part (§2.15.1, §2.13; R-24, R-25; cov2 C12): just after RCOP 2026's crown the world writes one final
 * line of at most 0.2 KB and no race line, and reloads to the same bytes; the final line is there an hour before the Afterglow's end and
 * gone at At + 3 days; the crown Honor's title is C1's fill; the mark line's bytes are printed (about 50 B, R-24, not asserted); a
 * podium mark's line carries its unpaid crown ticket's trailing t until FestTicketPaid drops it, once. On the day-66 world (FEIGN 2026's
 * Main resolved, R-25) the world\trecent records carry the 9th curve field only when it isn't Standard: every such record is the Main
 * curve's and a festival Main's, the rest have 8 fields, and the text reloads to the same bytes.
 * Planted bugs: FestLast never cleared; FestTicketPaid a no-op; the curve field written when Standard (G/WorldSave.cpp).
 */
void FestSaveRoundTripCrown()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		Expect(false, "FestSaveRoundTrip (crown): RCOP 2026 is in fest::All()");
		return;
	}
	const world::World& After = FestAfterWorld();
	const std::string Text = FestTextWorld(After);
	const std::string Final = FestLinesWorld(Text, "world\tfest\tfinal\t");
	const std::string Mark = FestLinesWorld(Text, "world\tfest\tmark\t");
	world::World Back;
	FestLoadWorld(Back, Text);
	const size_t FinalBytes = Final.empty() ? 0 : Final.size() - 1;
	std::printf("  FestSaveRoundTrip: after RCOP 2026's crown the final line is %zu B (cap 0.2 KB); the hero's mark line %zu B (about 50 B, R-24): %s", FinalBytes,
		Mark.empty() ? static_cast<size_t>(0) : Mark.size() - 1, Mark.empty() ? "(none)\n" : Mark.c_str());
	Expect(FestCountLinesWorld(Text, "world\tfest\tfinal\t") == 1 && FinalBytes <= 200 && FestCountLinesWorld(Text, "world\tfest\trace\t") == 0 && FestTextWorld(Back) == Text,
		"FestSaveRoundTrip: just after RCOP 2026's crown the world writes one final line of at most 0.2 KB (" + std::to_string(FinalBytes) + " B), no race line, and reloads to the same bytes");
	const std::vector<world::Honor> Crowns = FestCrownsOfWorld(After, "rcop");
	Expect(Crowns.size() == 1 && Crowns.front().Title == FestC1World(*Rcop),
		"FestSaveRoundTrip: the crown Honor's title is C1's fill ('" + (Crowns.empty() ? std::string() : Crowns.front().Title) + "', C1 '" + FestC1World(*Rcop) + "')");

	const double C = world::FestCrownedAt(After, "rcop");
	world::World Glow = After;
	Glow.AdvanceTo(C + 3.0 * 1440.0 - 60.0);
	const size_t HourBefore = FestCountLinesWorld(FestTextWorld(Glow), "world\tfest\tfinal\t");
	Glow.AdvanceTo(C + 3.0 * 1440.0);
	const std::string Gone = FestTextWorld(Glow);
	Expect(HourBefore == 1 && FestCountLinesWorld(Gone, "world\tfest\tfinal\t") == 0 && FestLinesWorld(Gone, "world\tfest\t") == Mark && world::FestRaceKey(Glow).empty(),
		"FestSaveRoundTrip: the final line lives the Afterglow: there an hour before " + FestWhenWorld(C + 3.0 * 1440.0) + " (" + std::to_string(HourBefore) +
			"), gone at At + 3 days (" + std::to_string(FestCountLinesWorld(Gone, "world\tfest\tfinal\t")) + "), the mark line kept");

	// The crown ticket's t (§2.13): a podium mark's record carries it until the session's FestStep pays it (FestTicketPaid, R-4: the
	// t dropped once, HeroRev moved so the session's next save writes the world; a second receipt, or another key's, changes nothing).
	world::World Podium;
	FestLoadWorld(Podium, FestTextWorld(*FestPeakWorld().W) + "world\tfest\trace\trcop\t12:5000:9,-1:4000:6,40:3000:4\n");
	Podium.AdvanceTo(FestHourAfterWorld(ss::fest::LastEnd(*Rcop)) + 30.0);
	const std::string Unpaid = FestLinesWorld(FestTextWorld(Podium), "world\tfest\tmark\t");
	const int RevBefore = Podium.HeroRevision();
	world::FestTicketPaid(Podium, "ignite.feign26");
	const bool OtherKept = Podium.HeroRevision() == RevBefore && FestLinesWorld(FestTextWorld(Podium), "world\tfest\tmark\t") == Unpaid;
	world::FestTicketPaid(Podium, "rcop");
	const int RevPaid = Podium.HeroRevision();
	const std::string Paid = FestLinesWorld(FestTextWorld(Podium), "world\tfest\tmark\t");
	world::FestTicketPaid(Podium, "rcop");
	world::World PaidBack;
	FestLoadWorld(PaidBack, FestTextWorld(Podium));
	const world::HeroFestMark* Pm = FestMarkOfWorld(PaidBack, "rcop");
	Expect(Unpaid == "world\tfest\tmark\trcop\t4\t2\t3\t4000\tt\n" && OtherKept && Paid == "world\tfest\tmark\trcop\t4\t2\t3\t4000\n" && RevPaid != RevBefore &&
			   Podium.HeroRevision() == RevPaid && Pm && !Pm->Ticket && Pm->Mark == world::FestMark::Podium,
		"FestSaveRoundTrip: a podium mark's line carries its unpaid crown ticket's t until FestTicketPaid, once (unpaid '" + FestLinesWorld(Unpaid, "world") + "', paid '" +
			FestLinesWorld(Paid, "world") + "', HeroRev " + std::to_string(RevBefore) + " -> " + std::to_string(RevPaid) + " -> " + std::to_string(Podium.HeroRevision()) + ")");

	FestFeignRunWorld& Feign = FestFeignWorld();
	const std::string Day66 = FestTextWorld(*Feign.W);
	int Curved = 0, Plain = 0, Bad = 0;
	for (const std::vector<std::string>& R : FestRecentWorld(Day66))
	{
		if (R.size() == 8)
		{
			++Plain;
		}
		else if (R.size() == 9 && R[8] == std::to_string(static_cast<int>(ss::PayoutCurve::Main)) && R[1].find("MAIN EVENT") != std::string::npos)
		{
			++Curved;
		}
		else
		{
			++Bad;
		}
	}
	world::World Back66;
	FestLoadWorld(Back66, Day66);
	std::printf("  FestSaveRoundTrip: the day-66 world's recent records: %d with the Main curve's 9th field (FEIGN 2026's Main), %d with 8 fields, %d other\n", Curved, Plain, Bad);
	Expect(Feign.Found && Curved > 0 && Bad == 0 && FestTextWorld(Back66) == Day66,
		"FestSaveRoundTrip: on the day-66 world the recent curve field is written only when it isn't Standard (" + std::to_string(Curved) + " Main-curve records of FEIGN 2026's Main, " +
			std::to_string(Plain) + " with 8 fields, " + std::to_string(Bad) + " other) and the text reloads to the same bytes");
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
		FestSaveRoundTripCrown(); // FW-2b's part
	}
	// FW-2b: the crown
	if (Only("FestCrownSkipInvariant"))
	{
		FestCrownSkipInvariant();
	}
	if (Only("FestCrownWaitsForHero"))
	{
		FestCrownWaitsForHero();
	}
	if (Only("FestNoRecrown"))
	{
		FestNoRecrown();
	}
	if (Only("FestHallScoreExcludesCrown"))
	{
		FestHallScoreExcludesCrown();
	}
	if (Only("FestAwardRebuildIgnoresCrown"))
	{
		FestAwardRebuildIgnoresCrown();
	}
	if (Only("FestCalendarFacts"))
	{
		FestCalendarFactsWorld(); // F-world's world half (F-net's in festivals_main.cpp)
	}
}
} // namespace festivals_test
