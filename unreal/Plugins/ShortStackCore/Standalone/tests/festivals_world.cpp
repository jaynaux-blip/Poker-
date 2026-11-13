// festivals: the world's side of Festival Season (update 4 SPEC v1.7 §2.15.1; F-world's file): the race and its marks, the crown and
// its tickets, the roll and the defender, Discover widened, the saves' festival lines, texts and letters, the Pass's first edition on
// two rents, the agenda merge, the Rerun keys. Called from festivals_main.cpp (its kit: Expect).
// Phase A (§6 PA-48): registered, empty. G1: F-world's checks land package by package (FW-1: FestReignCopy, the strings and the
// sponsor half; FW-2a: the race and its marks: FestRaceDeterministic, FestRaceTiesToPerson, FestMarksNested, FestLadderCutoff,
// FestReloadMidRace, FestOldSaveMidFestival and FestSaveRoundTrip's race part, with the race's budget lines; FW-2b: the crown:
// FestCrownSkipInvariant, FestCrownWaitsForHero, FestNoRecrown, FestHallScoreExcludesCrown, FestAwardRebuildIgnoresCrown,
// FestSaveRoundTrip's final line and day-66 curve field, FestCalendarFacts' world half, with the crown's budget lines; FW-3: the titles:
// FestRollFromHonors, FestBlankHonesty, FestDefending, FestDiscoverWidened, FestHeroPinsDerived (with HeroFestPins' budget line),
// FestHeadline, FestTitleOrder, and FestChampTitles' and FestMajorFloor's world halves; FW-4: WHO'S IN: FestFieldStories,
// FestStoryHooks, on Honors and fields planted through the world text; FW-5: the session's texts, moments and Rerun: FestTexts,
// FestMoments, FestRerunStories, through SessionFestDetail.h and one session run on marks planted through its world; FW-6: FestStep's
// tickets, refunds and letter and ss.Fest.*: FestCrownTickets (a session run into RCOP 2026's window, its marks planted), FestStepCredit
// (FestStepCredits at any minute on a world with crowns planted), FestMainTicket's world half). Every helper here
// is in an anonymous namespace (PLAN_G1 §1 item 8: festivals_main.cpp shares the executable); each check runs alone as "festivals_test
// <Check>" (the kit's Only, FN-1). The race checks build one Blank world before RCOP 2026 opens and run it through the edition (about
// 18 ms a world day), planting races through the world text; the crown's run it on through FEIGN 2026, the first rival festival, to
// day 67, never past FEIGN I (§8.1: "three Blank worlds to day 65").
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SessionFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoContentPriv.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldSim.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace fest
{
namespace fest_text_detail
{
const char* FestText(const std::string& Key); // G/SessionFest.cpp: the festival texts' one table, by key
} // namespace fest_text_detail
} // namespace fest
namespace video
{
namespace video_copy
{
std::vector<std::string> FestStoryLines(const std::string& Event, TitleStyle T, const std::string& IdeaFacts); // G/VideoCopy.cpp (FW-5)
} // namespace video_copy
} // namespace video
} // namespace ss

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

// ------------------------------------------------------------------ FW-3: the roll, the defender, the titles, Discover, the pins and
// the headlines (§2.6.2-§2.6.5, §2.6.8, §2.9.2, §2.15.1). Later editions' Honors, results and wins are planted (through the world text,
// or one event resolved as the world resolves it: Sim::OnlineEvent, Sim::Resolve), never simulated past FEIGN I (§8.1).

/** Brand B's edition of Year (nullptr: none in fest::All()). */
const ss::fest::Festival* FestEditionWorld(ss::fest::Brand B, int Year)
{
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Which == B && F.Year == Year)
		{
			return &F;
		}
	}
	return nullptr;
}

/** The first instance of template Id in edition F's window (its days and the next, for a late finish). */
bool FestInstanceWorld(const ss::fest::Festival& F, const std::string& Id, ss::net::EventInstance& Out)
{
	const ss::net::Network& N = ss::net::Site(F.Site);
	for (const ss::net::EventInstance& E : N.Window(FestAtWorld(F.FirstDay), FestAtWorld(F.LastDay + 1)))
	{
		if (N.TemplateOf(E).Id == Id)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

/** Instance E resolved on W as the world resolves its events (Sim::OnlineEvent, then Sim::Resolve), its winner planted: a person
 *  (Winner >= 0, their place known), the hero (-1, HeroPlace 1) or an unknown (-2: nobody the world follows in the field). Major marks
 *  the pending a major (a $1M pool's), for a run that is none. Returns the pending as resolved. */
ss::world::Pending FestResolveWorld(ss::world::World& W, const ss::net::EventInstance& E, int Winner, bool Major = false)
{
	ss::world::Pending P = ss::world::Sim::OnlineEvent(W, E);
	P.Who.clear();
	P.HeroPlace = 0;
	P.HeroPrize = 0;
	if (Winner >= 0)
	{
		ss::world::Entry In;
		In.Npc = Winner;
		In.Known = 1;
		P.Who.push_back(In);
	}
	else if (Winner == -1)
	{
		P.HeroPlace = 1;
		P.HeroPrize = P.Pool / 5;
	}
	P.Major = P.Major || Major;
	ss::world::Sim::Resolve(W, P);
	return P;
}

/** A person the world follows: the first id from From up who is in the world and not faded (-1: none). */
int FestPersonWorld(const ss::world::World& W, int From = 1)
{
	for (int K = From; K < static_cast<int>(W.People().size()); ++K)
	{
		const ss::world::Npc* N = W.Get(K);
		if (N && !N->Faded)
		{
			return K;
		}
	}
	return -1;
}

/** The world's first post of kind K at minute At by npc Npc (nullptr: none). */
const ss::world::WorldEvent* FestPostWorld(const ss::world::World& W, ss::world::EventKind K, double At, int Npc)
{
	for (const ss::world::WorldEvent& E : W.Events())
	{
		if (E.Kind == K && E.At == At && E.Npc == Npc)
		{
			return &E;
		}
	}
	return nullptr;
}

/** The world's newest Honor with EventId Id (nullptr: none; an event resolved again on a copy has two). */
const ss::world::Honor* FestHonorWorld(const ss::world::World& W, const std::string& Id)
{
	const std::vector<ss::world::Honor>& Honors = W.Honors();
	for (auto It = Honors.rbegin(); It != Honors.rend(); ++It)
	{
		if (It->EventId == Id)
		{
			return &*It;
		}
	}
	return nullptr;
}

/** Whether S starts with Head. */
bool FestStartsWorld(const std::string& S, const std::string& Head)
{
	return S.compare(0, Head.size(), Head) == 0;
}

/** A world\thonor line as the save writes it (year, title, event id, npc, name, prize, entries, minute). */
std::string FestHonorLineWorld(int Year, const std::string& Title, const std::string& EventId, int Npc, const std::string& Name, long long Prize, int Entries, double At)
{
	return "world\thonor\t" + std::to_string(Year) + "\t" + Title + "\t" + EventId + "\t" + std::to_string(Npc) + "\t" + Name + "\t" + std::to_string(Prize) + "\t" +
		   std::to_string(Entries) + "\t" + std::to_string(At) + "\n";
}

/** Text with Lines put before its first world\thonor line (first in history), or at its end when it has none. */
std::string FestPlantFirstWorld(std::string Text, const std::string& Lines)
{
	const size_t First = Text.find("world\thonor\t");
	Text.insert(First == std::string::npos ? Text.size() : First, Lines);
	return Text;
}

/** A template's printed name on its edition's site ("" when it has none). */
std::string FestTemplateNameWorld(const ss::fest::Festival& F, const std::string& Id)
{
	const ss::net::EventTemplate* T = ss::net::Site(F.Site).FindTemplate(Id);
	return T ? T->Name : std::string();
}

/** An edition's Main event id at its start minute ("<MainId>@<minute>"). */
std::string FestMainIdWorld(const ss::fest::Festival& F)
{
	return F.MainId + "@" + std::to_string(std::llround(F.MainStart));
}

/** An Honor's template id (before '@'). */
std::string FestTemplateWorld(const ss::world::Honor& H)
{
	return H.EventId.substr(0, H.EventId.rfind('@'));
}

/** Whether every row of brand B's roll holds only its edition's titles and belt (the Main its MainId, the Mini and Micro other TitleIds,
 *  the belt "<series>#pos"), none before Night One, newest edition first; Why names the first row that doesn't. */
bool FestRollCleanWorld(const ss::world::World& W, ss::fest::Brand B, std::string& Why)
{
	int Year = 1 << 30;
	const double NightOne = FestAtWorld(ss::net::NightOneDay);
	for (const ss::world::FestRollRow& R : ss::world::FestRoll(W, B))
	{
		const ss::fest::Festival* F = ss::fest::FestivalOf(R.Key);
		const std::vector<std::string> Titles = F ? F->TitleIds : std::vector<std::string>();
		auto Other = [&](const ss::world::Honor* H) {
			return !H || (!Titles.empty() && FestTemplateWorld(*H) != Titles[0] && std::find(Titles.begin(), Titles.end(), FestTemplateWorld(*H)) != Titles.end() && H->At >= NightOne);
		};
		const bool Main = !R.Main || (!Titles.empty() && FestTemplateWorld(*R.Main) == Titles[0] && R.Main->At >= NightOne);
		const bool Crown = !R.Crown || (R.Crown->EventId == R.Key + "#pos" && R.Crown->At >= NightOne);
		if (!F || F->Which != B || !Main || !Other(R.Mini) || !Other(R.Micro) || !Crown || R.Year > Year || (!R.Main && !R.Mini && !R.Micro && !R.Crown))
		{
			Why = R.Key + " " + std::to_string(R.Year);
			return false;
		}
		Year = R.Year;
	}
	return true;
}

/**
 * FestRollFromHonors (§2.15.1, §2.6.2): the roll holds only Honors whose template is in their edition's TitleIds, and crown Honors, by
 * edition, newest first. RCOP 2026 run for real (the skip world, day 50) reads its four lines (Main, Mini, Micro, the belt). Planted
 * through the world text: an unknown's PENNANT 2027 Main title (an old save's, Discover not yet widened) stands on PENNANT's roll, and an
 * RCOP 2026 numbered event's Honor, first in history, stands on no roll. Every brand's rows hold their edition's titles and belt only,
 * none before Night One. A Blank and an Established world run from day 12 to day 60 read the same RCOP roll (the same events, only
 * what happened in each).
 * Planted bugs: an unknown's Honor dropped; a numbered event's Honor on the roll.
 */
void FestRollFromHonors()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	ss::net::EventInstance Numbered;
	if (!Rcop || !Pen || Rcop->TitleIds.size() != 3 || !FestHeroEventWorld(Numbered))
	{
		Expect(false, "FestRollFromHonors: RCOP 2026 (three titles), PENNANT 2027 and an RCOP 2026 numbered event exist");
		return;
	}
	const world::World& Skip = *FestSkipWorld().Skip;
	const int Who = FestPersonWorld(Skip);
	const std::string Planted =
		FestHonorLineWorld(2026, ss::net::Shared().TemplateOf(Numbered).Name, Numbered.Id, Who, Skip.Get(Who)->Name, 120000000, Numbered.Entries, Numbered.Start + 600.0) +
		FestHonorLineWorld(2027, FestTemplateNameWorld(*Pen, Pen->MainId), FestMainIdWorld(*Pen), -2, "Lee Marlow", 1950000, 420, Pen->MainStart + 600.0);
	world::World W;
	FestLoadWorld(W, FestPlantFirstWorld(FestTextWorld(Skip), Planted));
	const std::vector<world::FestRollRow> Roll = world::FestRoll(W, Brand::Rcop);
	const bool Four = Roll.size() == 1 && Roll[0].Key == "rcop" && Roll[0].Year == 2026 && Roll[0].Main && Roll[0].Mini && Roll[0].Micro && Roll[0].Crown &&
					  FestTemplateWorld(*Roll[0].Main) == Rcop->TitleIds[0] && FestTemplateWorld(*Roll[0].Mini) == Rcop->TitleIds[1] &&
					  FestTemplateWorld(*Roll[0].Micro) == Rcop->TitleIds[2] && Roll[0].Crown->EventId == "rcop#pos";
	Expect(Four, "FestRollFromHonors: RCOP 2026's row holds its Main, Mini and Micro Main Event titles and its belt, and not the planted numbered event's Honor (" +
					 std::to_string(Roll.size()) + " row(s)" + (Roll.empty() || !Roll[0].Main ? std::string() : ", Main " + Roll[0].Main->EventId) + ")");
	const std::vector<world::FestRollRow> PenRoll = world::FestRoll(W, Brand::Pennant);
	Expect(PenRoll.size() == 1 && PenRoll[0].Key == Pen->SeriesId && PenRoll[0].Main && PenRoll[0].Main->Npc == -2 && PenRoll[0].Main->Name == "Lee Marlow",
		"FestRollFromHonors: an unknown's planted PENNANT 2027 Main title stands on PENNANT's roll (" + std::to_string(PenRoll.size()) + " row(s))");
	bool Clean = true;
	bool Placed = false;
	std::string Why;
	size_t Rows = 0;
	for (int B = 1; B < static_cast<int>(Brand::Count); ++B)
	{
		Clean = Clean && FestRollCleanWorld(W, static_cast<Brand>(B), Why);
		for (const world::FestRollRow& R : world::FestRoll(W, static_cast<Brand>(B)))
		{
			++Rows;
			for (const world::Honor* H : {R.Main, R.Mini, R.Micro, R.Crown})
			{
				Placed = Placed || (H && H->EventId == Numbered.Id);
			}
		}
	}
	Expect(Clean && !Placed, "FestRollFromHonors: every brand's roll (" + std::to_string(Rows) + " rows) holds only its edition's titles and belt, newest first, none before Night One" +
								 (Why.empty() ? std::string() : " (first bad row: " + Why + ")") + (Placed ? " (the numbered event's Honor is on a roll)" : ""));
	// Blank and Established read the same: only what happened in this world (U5).
	const auto T0 = std::chrono::steady_clock::now();
	world::World Blank = Skip;
	Blank.AdvanceTo(FestAtWorld(60));
	world::World Est;
	Est.Create(kFestSeedWorld, 12.0 * 1440.0 + 360.0, world::Start::Established);
	Est.AdvanceTo(FestAtWorld(60));
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	const std::vector<world::FestRollRow> A = world::FestRoll(Blank, Brand::Rcop);
	const std::vector<world::FestRollRow> E = world::FestRoll(Est, Brand::Rcop);
	auto Id = [](const world::Honor* H) { return H ? H->EventId : std::string("-"); };
	bool Same = A.size() == 1 && A.size() == E.size();
	for (size_t K = 0; Same && K < A.size(); ++K)
	{
		Same = A[K].Key == E[K].Key && Id(A[K].Main) == Id(E[K].Main) && Id(A[K].Mini) == Id(E[K].Mini) && Id(A[K].Micro) == Id(E[K].Micro) && Id(A[K].Crown) == Id(E[K].Crown) &&
			   A[K].Main && A[K].Mini && A[K].Micro && A[K].Crown;
	}
	std::printf("  FestRollFromHonors: day 60 Blank and Established (made and run in %.0f ms): RCOP rows %zu and %zu; Main winners %s / %s\n", Ms, A.size(), E.size(),
		A.empty() || !A[0].Main ? "-" : A[0].Main->Name.c_str(), E.empty() || !E[0].Main ? "-" : E[0].Main->Name.c_str());
	std::string WhyA, WhyE;
	const bool CleanA = FestRollCleanWorld(Blank, Brand::Rcop, WhyA);
	const bool CleanE = FestRollCleanWorld(Est, Brand::Rcop, WhyE);
	Expect(Same && CleanA && CleanE, "FestRollFromHonors: a Blank and an Established world at day 60 show the same RCOP roll (the same four events, nothing before Night One: " +
										 std::to_string(A.size()) + " and " + std::to_string(E.size()) + " rows)");
}

/**
 * FestBlankHonesty (§2.15.1, item 8): before an RCOP Main Honor exists the hub's data has no "Last year" figure (no roll row, no defender
 * of either kind on any day; RCOP 2026 mid-race, day 20); after it (day 50) RCOP 2027's "Last year" (FestDefending's Main) is that Honor,
 * its prize the one the world paid (the Champion post's Amount).
 * Planted bug: a constant prize (Lasting's Honor keeping a typed figure).
 */
void FestBlankHonesty()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	if (!Rcop)
	{
		Expect(false, "FestBlankHonesty: RCOP 2026 is in fest::All()");
		return;
	}
	const world::World& Mid = FestMidWorld();
	bool None = world::FestRoll(Mid, Brand::Rcop).empty();
	for (int Day = 20; Day <= 420; Day += 5)
	{
		None = None && !world::FestDefending(Mid, Brand::Rcop, false, Day) && !world::FestDefending(Mid, Brand::Rcop, true, Day);
	}
	Expect(None, "FestBlankHonesty: before RCOP 2026's Main has an Honor (day 20) the roll is empty and no day reads a defender (no \"Last year\")");
	const world::World& Skip = *FestSkipWorld().Skip;
	const world::Honor* Last = world::FestDefending(Skip, Brand::Rcop, false, 50);
	const world::WorldEvent* Post = Last ? FestPostWorld(Skip, world::EventKind::Champion, Last->At, Last->Npc) : nullptr;
	const std::vector<world::FestRollRow> Roll = world::FestRoll(Skip, Brand::Rcop);
	Expect(Last && FestStartsWorld(Last->EventId, Rcop->MainId + "@") && !Roll.empty() && Roll[0].Main == Last && Post && Post->What == Last->Title && Last->Prize > 0 &&
			   Last->Prize == Post->Amount,
		"FestBlankHonesty: after it, RCOP 2027's \"Last year\" is RCOP 2026's Main Honor and its prize is the one the world paid (" +
			(Last ? std::to_string(Last->Prize) : std::string("no Honor")) + " vs the post's " + (Post ? std::to_string(Post->Amount) : std::string("none")) + ")");
}

/**
 * FestDefending (§2.15.1, §2.6.3): RCOP 2026 run for real (day 50) and RCOP 2027's Main title and belt planted (its Main's end; the
 * first hour after its LastEnd): RCOP 2026's Main champion defends from their Honor through RCOP 2027's first day and its last, until
 * RCOP 2027 crowns; then RCOP 2027's champion does. The belt the same. RCOP 2027 never crowned (no belt planted): RCOP 2026's belt holder
 * defends through its last day and not two days past its last event (R-13). IsDefending asks the Main. The defender is not registered
 * for RCOP 2027's Main in this world: the hub's card and WHO'S IN show them only when World::Registered says so (F-hubs, FW-4).
 * Planted bug: the window closing at the next edition's first day.
 */
void FestDefending()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* R27 = FestEditionWorld(Brand::Rcop, 2027);
	const world::World& Skip = *FestSkipWorld().Skip;
	const std::vector<world::FestRollRow> Roll = world::FestRoll(Skip, Brand::Rcop);
	if (!Rcop || !R27 || Roll.size() != 1 || !Roll[0].Main || !Roll[0].Crown || Roll[0].Main->Npc < 0)
	{
		Expect(false, "FestDefending: RCOP 2026 run with its Main champion (a person) and belt; RCOP 2027 in fest::All()");
		return;
	}
	const int Champ = Roll[0].Main->Npc;
	const int Belt = Roll[0].Crown->Npc;
	const int Champ27 = FestPersonWorld(Skip, std::max(Champ, Belt) + 1);
	const int Belt27 = FestPersonWorld(Skip, Champ27 + 1);
	const double Crown27 = FestHourAfterWorld(ss::fest::LastEnd(*R27));
	const int CrownDay = static_cast<int>(Crown27 / 1440.0);
	const std::string Main27 = FestHonorLineWorld(2027, FestTemplateNameWorld(*R27, R27->MainId), FestMainIdWorld(*R27), Champ27, Skip.Get(Champ27)->Name, 300000000, 5100,
		R27->MainStart + 600.0);
	const std::string Belt27Line = FestHonorLineWorld(2027, R27->Name + " Player of the Series", R27->SeriesId + "#pos", Belt27, Skip.Get(Belt27)->Name, 0, 6100, Crown27);
	world::World W;
	FestLoadWorld(W, FestTextWorld(Skip) + Main27 + Belt27Line);
	auto Npc = [&](bool Crown, int Day) {
		const world::Honor* H = world::FestDefending(W, Brand::Rcop, Crown, Day);
		return H ? H->Npc : -9;
	};
	const int Days[5] = {50, R27->FirstDay, R27->LastDay, CrownDay, CrownDay + 30};
	const int WantMain[5] = {Champ, Champ, Champ, Champ27, Champ27};
	const int WantBelt[5] = {Belt, Belt, Belt, Belt27, Belt27};
	std::string Got;
	bool Holds = true;
	for (int K = 0; K < 5; ++K)
	{
		Holds = Holds && Npc(false, Days[K]) == WantMain[K] && Npc(true, Days[K]) == WantBelt[K] && world::IsDefending(W, WantMain[K], Days[K]) &&
				world::IsDefending(W, Champ, Days[K]) == (WantMain[K] == Champ) && !world::IsDefending(W, -2, Days[K]);
		Got += " day " + std::to_string(Days[K]) + ": " + std::to_string(Npc(false, Days[K])) + "/" + std::to_string(Npc(true, Days[K]));
	}
	Expect(Holds, "FestDefending: RCOP 2026's champion (npc " + std::to_string(Champ) + ") and belt holder (" + std::to_string(Belt) +
					  ") defend from their Honors until RCOP 2027 crowns (day " + std::to_string(CrownDay) + "), then its champion and belt holder do; Main/belt by day:" + Got);
	world::World Never;
	FestLoadWorld(Never, FestTextWorld(Skip) + Main27);
	const int Off = static_cast<int>((ss::fest::LastEnd(*R27) + 2.0 * 1440.0) / 1440.0) + 1;
	const world::Honor* Held = world::FestDefending(Never, Brand::Rcop, true, R27->LastDay + 1);
	const world::Honor* Gone = world::FestDefending(Never, Brand::Rcop, true, Off);
	Expect(Held && Held->Npc == Belt && !Gone, "FestDefending: RCOP 2027 never crowned: RCOP 2026's belt holder defends through its last day and not two days past its last event");
	world::World Reg = W;
	const std::vector<int> In = Reg.Registered(FestMainIdWorld(*R27));
	Expect(std::find(In.begin(), In.end(), Champ) == In.end(), "FestDefending: the defender is not registered for RCOP 2027's Main in this world (the card and WHO'S IN gate on it)");
}

/**
 * FestDiscoverWidened (§2.15.1, §2.6.5): an unknown who wins a festival title becomes a world person that night with one result and a
 * Discovered post: FEIGN 2026's Main (a rival festival Main, under $1M), PENNANT 2027's Mini and Micro Main Events and RCOP 2026's Mini
 * (a championship's, RCOP's included; each resolved on a copy of the skip world as the world resolves an event); Summer Slam 2027's Mini
 * (a major but no title) leaves its unknown winner unknown.
 * Planted bugs: the term removed; FestivalMain kept in place of FestivalTitle.
 */
void FestDiscoverWidened()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Slam = FestEditionWorld(Brand::Slam, 2027);
	if (!Feign || !Pen || !Rcop || !Slam || Pen->TitleIds.size() != 3 || Rcop->TitleIds.size() != 3)
	{
		Expect(false, "FestDiscoverWidened: FEIGN 2026, PENNANT 2027 (three titles), RCOP 2026 and Summer Slam 2027 are in fest::All()");
		return;
	}
	const std::string SlamMini = Slam->MainId.substr(0, Slam->MainId.size() - 4) + "mini";
	const std::pair<const ss::fest::Festival*, std::string> Cases[5] = {
		{Feign, Feign->MainId}, {Pen, Pen->TitleIds[1]}, {Pen, Pen->TitleIds[2]}, {Rcop, Rcop->TitleIds[1]}, {Slam, SlamMini}};
	for (int K = 0; K < 5; ++K)
	{
		const bool Want = K < 4;
		ss::net::EventInstance E;
		if (!FestInstanceWorld(*Cases[K].first, Cases[K].second, E))
		{
			Expect(false, "FestDiscoverWidened: an instance of " + Cases[K].second + " in " + Cases[K].first->Name);
			continue;
		}
		world::World W = *FestSkipWorld().Skip;
		const size_t Before = W.People().size();
		const world::Pending P = FestResolveWorld(W, E, -2);
		const int New = static_cast<int>(W.People().size()) - 1;
		const world::Npc* N = W.People().size() == Before + 1 ? W.Get(New) : nullptr;
		const world::WorldEvent* Found = N ? FestPostWorld(W, world::EventKind::Discovered, P.End, New) : nullptr;
		const world::Honor* H = FestHonorWorld(W, P.Id);
		const bool Made = N && Found && Found->What == P.Name && N->Totals[0].Events == 1 && N->Totals[0].Wins == 1 && H && H->Npc == New;
		const bool Unknown = W.People().size() == Before && (!H || H->Npc == -2);
		Expect(Want ? Made : Unknown, "FestDiscoverWidened: an unknown who wins " + P.Name + " (" + Cases[K].first->Name + ") " +
										  (Want ? "becomes a world person with one result and a Discovered post" : "stays unknown (no title: no Discover)") + " (" +
										  std::to_string(W.People().size() - Before) + " made, Honor npc " + (H ? std::to_string(H->Npc) : std::string("none")) + ")");
	}
}

/** One planted hero result line (world\theroresult: event id, place, prize). */
std::string FestHeroResultLineWorld(const std::string& Id, int Place, long long Prize)
{
	return "world\theroresult\t" + Id + "\t" + std::to_string(Place) + "\t" + std::to_string(Prize) + "\n";
}

/**
 * FestHeroPinsDerived (§2.15.1, §2.6.4): planted hero results (world\theroresult lines on the day-20 world) give RCOP 2026 a gold pin (a
 * win, a cash and a bust in its numbered events: 3 events, 2 cashes, best 1st of the won event's field) and FEIGN 2026 an outline pin
 * (a bust; its satellite win and a weekly never count) and the first later edition with a numbered event a silver pin (one cash, 12th,
 * no win), in calendar order; HeroFestResults lists RCOP 2026's three newest first with their fields, the silver edition's one and
 * none for an edition never played. The budget: HeroFestPins over 5,000 planted results (printed; <= 1 ms asserted under SS_PERF=1,
 * median of five rebuilds).
 * Planted bugs: satellites counted; a cashed edition reading outline.
 */
void FestHeroPinsDerived()
{
	namespace world = ss::world;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	if (!Rcop || !Feign)
	{
		Expect(false, "FestHeroPinsDerived: RCOP 2026 and FEIGN 2026 are in fest::All()");
		return;
	}
	const ss::net::Network& Rl = ss::net::Shared();
	const ss::net::Network& Ig = ss::net::Site(Feign->Site);
	std::vector<ss::net::EventInstance> Numbered;
	ss::net::EventInstance Weekly;
	bool HaveWeekly = false;
	for (const ss::net::EventInstance& E : Rl.Window(FestAtWorld(14), FestAtWorld(17)))
	{
		const ss::net::EventTemplate& T = Rl.TemplateOf(E);
		if (T.Series == "rcop" && T.Fmt != ss::net::Format::Satellite && E.Entries >= 50 && Numbered.size() < 3)
		{
			Numbered.push_back(E);
		}
		if (T.Series.empty() && !HaveWeekly)
		{
			Weekly = E;
			HaveWeekly = true;
		}
	}
	ss::net::EventInstance Bust, Sat;
	bool HaveBust = false, HaveSat = false;
	for (const ss::net::EventInstance& E : Ig.Window(FestAtWorld(Feign->FirstDay), FestAtWorld(Feign->LastDay + 1)))
	{
		const ss::net::EventTemplate& T = Ig.TemplateOf(E);
		if (T.Series == Feign->SeriesId && T.Fmt != ss::net::Format::Satellite && T.Id != Feign->MainId && !HaveBust)
		{
			Bust = E;
			HaveBust = true;
		}
	}
	// One of the edition's satellites (a satellite has no series, SPEC §2.3.4: Festival::Satellites lists them, as the Ignite floor reads
	// its rows).
	HaveSat = !Feign->Satellites.empty() && FestInstanceWorld(*Feign, Feign->Satellites.front(), Sat) && Ig.TemplateOf(Sat).Fmt == ss::net::Format::Satellite;
	// The silver pin: one cash and no win in a third edition, the first after FEIGN 2026 in fest::All() (calendar order) with a numbered
	// event of its series in its days.
	const ss::fest::Festival* Third = nullptr;
	ss::net::EventInstance Cash;
	bool PastFeign = false;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (Third)
		{
			break;
		}
		if (&F == Feign)
		{
			PastFeign = true;
			continue;
		}
		if (!PastFeign || &F == Rcop || F.SeriesId.empty())
		{
			continue;
		}
		const ss::net::Network& N = ss::net::Site(F.Site);
		for (const ss::net::EventInstance& E : N.Window(FestAtWorld(F.FirstDay), FestAtWorld(F.LastDay + 1)))
		{
			const ss::net::EventTemplate& T = N.TemplateOf(E);
			if (T.Series == F.SeriesId && T.Fmt != ss::net::Format::Satellite && T.Id != F.MainId && E.Entries >= 50)
			{
				Third = &F;
				Cash = E;
				break;
			}
		}
	}
	if (Numbered.size() < 3 || !HaveWeekly || !HaveBust || !HaveSat || !Third)
	{
		Expect(false, "FestHeroPinsDerived: three RCOP 2026 numbered events, a weekly, a FEIGN 2026 event, one of its satellites and a later edition's event (" +
						  std::to_string(Numbered.size()) + " numbered, weekly " + (HaveWeekly ? "yes" : "no") + ", event " + (HaveBust ? "yes" : "no") + ", satellite " +
						  (HaveSat ? "yes" : "no") + ", later edition " + (Third ? "yes" : "no") + ")");
		return;
	}
	const std::string Lines = FestHeroResultLineWorld(Numbered[0].Id, 1, 150000) + FestHeroResultLineWorld(Numbered[1].Id, 30, 2400) +
							  FestHeroResultLineWorld(Numbered[2].Id, 900, 0) + FestHeroResultLineWorld(Bust.Id, 700, 0) + FestHeroResultLineWorld(Sat.Id, 1, 11000) +
							  FestHeroResultLineWorld(Weekly.Id, 1, 50000) + FestHeroResultLineWorld(Cash.Id, 12, 3100);
	world::World W;
	FestLoadWorld(W, FestTextWorld(FestMidWorld()) + Lines);
	const std::vector<world::HeroFestPin> Pins = world::HeroFestPins(W);
	const bool Three = Pins.size() == 3 && Pins[0].Key == "rcop" && Pins[1].Key == Feign->SeriesId && Pins[2].Key == Third->SeriesId;
	const bool Gold = Three && Pins[0].Grade == 3 && Pins[0].Events == 3 && Pins[0].Cashes == 2 && Pins[0].Wins == 1 && Pins[0].BestPlace == 1 && Pins[0].BestEntries == Numbered[0].Entries;
	const bool Outline = Three && Pins[1].Grade == 1 && Pins[1].Events == 1 && Pins[1].Cashes == 0 && Pins[1].Wins == 0 && Pins[1].BestPlace == 700 && Pins[1].BestEntries == Bust.Entries;
	const bool Silver = Three && Pins[2].Grade == 2 && Pins[2].Events == 1 && Pins[2].Cashes == 1 && Pins[2].Wins == 0 && Pins[2].BestPlace == 12 && Pins[2].BestEntries == Cash.Entries;
	Expect(Gold && Outline && Silver, "FestHeroPinsDerived: RCOP 2026 gold (3 events, 2 cashes, 1 win, best 1st of " + std::to_string(Numbered[0].Entries) +
										  "), FEIGN 2026 outline (its satellite win and a weekly never count) and " + Third->SeriesId + " silver (1 cash, no win, best 12th of " +
										  std::to_string(Cash.Entries) + "): " + std::to_string(Pins.size()) + " pin(s)" +
										  (Pins.empty() ? std::string() : ", the first " + Pins[0].Key + " grade " + std::to_string(Pins[0].Grade) + " events " + std::to_string(Pins[0].Events)) +
										  (Pins.size() < 2 ? std::string() : ", the second grade " + std::to_string(Pins[1].Grade) + " events " + std::to_string(Pins[1].Events)) +
										  (Pins.size() < 3 ? std::string() : ", the third " + Pins[2].Key + " grade " + std::to_string(Pins[2].Grade) + " cashes " + std::to_string(Pins[2].Cashes)));
	const std::vector<world::HeroFestResult> Rows = world::HeroFestResults(W, "rcop");
	bool Newest = Rows.size() == 3 && Numbered[0].Start <= Numbered[1].Start && Numbered[1].Start <= Numbered[2].Start;
	for (size_t K = 0; Newest && K < Rows.size(); ++K)
	{
		const size_t I = 2 - K; // Numbered is in start order
		Newest = Rows[K].EventId == Numbered[I].Id && Rows[K].Entries == Numbered[I].Entries && Rows[K].Place == (I == 0 ? 1 : I == 1 ? 30 : 900);
	}
	// The silver edition's one row; an edition the hero never played has none.
	const std::vector<world::HeroFestResult> CashRows = world::HeroFestResults(W, Third->SeriesId);
	const bool CashRow = CashRows.size() == 1 && CashRows[0].EventId == Cash.Id && CashRows[0].Place == 12 && CashRows[0].Prize == 3100 && CashRows[0].Entries == Cash.Entries;
	std::string Unplayed;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (Unplayed.empty() && &F != Rcop && &F != Feign && &F != Third && !F.SeriesId.empty())
		{
			Unplayed = F.SeriesId;
		}
	}
	Expect(Newest && CashRow && !Unplayed.empty() && world::HeroFestResults(W, Unplayed).empty(),
		"FestHeroPinsDerived: HeroFestResults lists RCOP 2026's three results newest first with their fields (" + std::to_string(Rows.size()) + " rows), " +
			Third->SeriesId + "'s one cash (" + std::to_string(CashRows.size()) + " rows) and none for " + Unplayed);
	// The budget: 5,000 planted results (RiverLine's from RCOP 2026's first day: its festival events and the weeklies), one rebuild timed on
	// each world copy.
	std::string Many;
	int Count = 0;
	for (const ss::net::EventInstance& E : Rl.Window(FestAtWorld(13), FestAtWorld(60)))
	{
		if (Count < 5000)
		{
			Many += FestHeroResultLineWorld(E.Id, 1 + Count % 400, Count % 3 == 0 ? 1000 : 0);
			++Count;
		}
	}
	world::World Big;
	FestLoadWorld(Big, FestTextWorld(FestMidWorld()) + Many);
	const std::vector<world::HeroFestPin> Warm = world::HeroFestPins(Big);
	std::vector<double> Ms;
	for (int Round = 0; Round < 5; ++Round)
	{
		const world::World Copy = Big;
		const auto T0 = std::chrono::steady_clock::now();
		const std::vector<world::HeroFestPin> Again = world::HeroFestPins(Copy);
		Ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count());
		(void)Again;
	}
	std::sort(Ms.begin(), Ms.end());
	const auto T1 = std::chrono::steady_clock::now();
	const std::vector<world::HeroFestPin> Cached = world::HeroFestPins(Big);
	const double Hit = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T1).count();
	std::printf("  FestHeroPinsDerived: HeroFestPins over %d planted results (%d of RCOP 2026's): a rebuild %.3f ms (median of 5; budget 1 ms), a cached read %.4f ms\n", Count,
		Warm.empty() ? 0 : Warm[0].Events, Ms[2], Hit);
	Expect(Count == 5000 && Warm.size() == 1 && Cached.size() == 1, "FestHeroPinsDerived: 5,000 planted results make RCOP 2026's one pin (" + std::to_string(Count) + " planted)");
	if (std::getenv("SS_PERF"))
	{
		Expect(Ms[2] <= 1.0, "FestHeroPinsDerived (SS_PERF): HeroFestPins over 5,000 results <= 1 ms a world revision (" + std::to_string(Ms[2]) + " ms)");
	}
}

/**
 * FestHeadline (§2.15.1, §2.9.2; FestNews' F-world half): the crown and festival-Main headlines fill (RCOP 2026's, just after its crown:
 * head:fest:crown with tag CROWN, head:fest:main naming The RCOP Cup with tag FESTIVAL; a Champion post that is no title keeps today's
 * line); a rival's crown (FEIGN 2026's) carries Ignite's bits, shows on Ignite's News and stays off RiverLine's; a planted
 * PENNANT 2027 Mini win posts head:fest:title (the Micro too); a GRIT run's Pending::Name, Honor and headline read its "-E" or "-L" (§8
 * Q22: Network::NameOf).
 * Planted bugs: site bits missing; the title branch falling to the default headline; Pending::Name reading TemplateOf(E).Name.
 */
void FestHeadline()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Rcop = FestRcopWorld();
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* Grit = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Grit = !Grit && F.Which == Brand::Grit ? &F : Grit;
	}
	if (!Rcop || !Pen || !Grit || Pen->TitleIds.size() != 3)
	{
		Expect(false, "FestHeadline: RCOP 2026, PENNANT 2027 (three titles) and a GRIT edition are in fest::All()");
		return;
	}
	std::string Title, Body, Tag;
	// RCOP 2026's crown and Main, on the world just after the crown.
	world::World After = FestAfterWorld();
	After.HeroName = "Tess Halloran";
	const std::vector<world::Honor> Crowns = FestCrownsOfWorld(After, "rcop");
	const std::vector<world::WorldEvent> CrownPosts = FestCrownPostsWorld(After, Rcop->Name);
	const world::Npc* Winner = Crowns.size() == 1 ? After.Get(Crowns[0].Npc) : nullptr;
	const std::string WinnerName = Winner ? Winner->Name : Crowns.size() == 1 && Crowns[0].Npc == -1 ? After.HeroName : std::string();
	const bool CrownHead = CrownPosts.size() == 1 && Crowns.size() == 1 && After.Headline(CrownPosts[0], Title, Body, Tag) &&
						   Title == WinnerName + " is the RCOP 2026 Player of the Series" &&
						   Body == WinnerName + " takes the RCOP 2026 belt: " + ss::Grouped(Crowns[0].Entries) + " points from " + ss::Grouped(CrownPosts[0].Amount) + " cashes." && Tag == "CROWN";
	Expect(CrownHead, "FestHeadline: RCOP 2026's crown reads head:fest:crown filled, tag CROWN ('" + Title + "' / '" + Body + "' / " + Tag + ")");
	const world::Honor* MainH = nullptr;
	for (const world::Honor& H : After.Honors())
	{
		MainH = !MainH && FestStartsWorld(H.EventId, Rcop->MainId + "@") ? &H : MainH;
	}
	const world::WorldEvent* MainPost = MainH ? FestPostWorld(After, world::EventKind::Champion, MainH->At, MainH->Npc) : nullptr;
	const world::Npc* MainWho = MainH ? After.Get(MainH->Npc) : nullptr;
	Title = Body = Tag = std::string();
	const bool MainHead = MainPost && MainWho && After.Headline(*MainPost, Title, Body, Tag) && Title == MainWho->Name + " wins the RCOP 2026 Main Event" && FestStartsWorld(Body, "$") &&
						  Body.find(" and The RCOP Cup, from a field of " + ss::Grouped(MainH->Entries) + ".") != std::string::npos && Tag == "FESTIVAL" && Body.find('{') == std::string::npos;
	Expect(MainHead, "FestHeadline: RCOP 2026's Main reads head:fest:main filled, tag FESTIVAL ('" + Title + "' / '" + Body + "' / " + Tag + ")");
	const world::WorldEvent* Plain = nullptr;
	for (const world::WorldEvent& E : After.Events())
	{
		world::FestSlot Slot = world::FestSlot::None;
		Plain = !Plain && E.Kind == world::EventKind::Champion && (E.Flags & world::FlagLive) == 0 && After.Get(E.Npc) && !world::FestTitlePost(After, E, Slot) ? &E : Plain;
	}
	Title = Body = Tag = std::string();
	Expect(Plain && After.Headline(*Plain, Title, Body, Tag) && Title == After.Get(Plain->Npc)->Name + " wins " + Plain->What && Tag == "CHAMPION",
		"FestHeadline: a Champion post that is no festival title keeps today's headline ('" + Title + "')");
	// FEIGN 2026's crown: Ignite's bits, Ignite's News, never RiverLine's.
	world::World Fw = *FestFeignWorld().W;
	Fw.HeroName = "Tess Halloran";
	const std::vector<world::WorldEvent> FeignPosts = FestCrownPostsWorld(Fw, "FEIGN 2026");
	const std::vector<world::Honor> FeignCrowns = FestCrownsOfWorld(Fw, "ignite.feign26");
	const world::Npc* FeignWinner = FeignCrowns.size() == 1 ? Fw.Get(FeignCrowns[0].Npc) : nullptr;
	const std::string FeignTitle =
		(FeignWinner ? FeignWinner->Name : FeignCrowns.size() == 1 && FeignCrowns[0].Npc == -1 ? Fw.HeroName : std::string("?")) + " is the FEIGN 2026 Player of the Series";
	Title = Body = Tag = std::string();
	const bool Bits = FeignPosts.size() == 1 && world::FlagSiteOf(FeignPosts[0].Flags) == ss::net::SiteId::Ignite && Fw.Headline(FeignPosts[0], Title, Body, Tag) && Title == FeignTitle;
	ss::net::AttachAll(&Fw);
	const ss::net::HeroStats NoHero;
	bool OnRl = false, OnIg = false;
	for (const ss::net::NewsItem& It : ss::net::Shared().News(Fw.Clock(), NoHero, 200))
	{
		OnRl = OnRl || It.Title.find("FEIGN 2026 Player of the Series") != std::string::npos;
	}
	for (const ss::net::NewsItem& It : ss::net::Site(ss::net::SiteId::Ignite).News(Fw.Clock(), NoHero, 200))
	{
		OnIg = OnIg || It.Title == FeignTitle;
	}
	ss::net::AttachAll(nullptr);
	Expect(Bits && OnIg && !OnRl, "FestHeadline: FEIGN 2026's crown carries Ignite's site bits, shows on Ignite's News and stays off RiverLine's (" + std::to_string(FeignPosts.size()) +
									  " post(s), '" + Title + "', on Ignite " + (OnIg ? "yes" : "no") + ", on RiverLine " + (OnRl ? "yes" : "no") + ")");
	// A championship's Mini and Micro Main Events: head:fest:title.
	for (size_t K = 1; K <= 2; ++K)
	{
		ss::net::EventInstance E;
		world::World W = *FestSkipWorld().Skip;
		const int Who = FestPersonWorld(W, 40 + static_cast<int>(K));
		if (!FestInstanceWorld(*Pen, Pen->TitleIds[K], E) || Who < 0)
		{
			Expect(false, "FestHeadline: an instance of " + Pen->TitleIds[K]);
			continue;
		}
		const world::Pending P = FestResolveWorld(W, E, Who);
		const world::WorldEvent* Post = FestPostWorld(W, P.Major ? world::EventKind::Champion : world::EventKind::Won, P.End, Who);
		Title = Body = Tag = std::string();
		const std::string Tier = K == 1 ? "Mini" : "Micro";
		Expect(Post && W.Headline(*Post, Title, Body, Tag) && Title == W.Get(Who)->Name + " wins the PENNANT 2027 " + Tier + " Main Event" &&
				   Body.find(" and a PENNANT 2027 title, from a field of " + ss::Grouped(P.Entries) + ".") != std::string::npos && Tag == "FESTIVAL",
			"FestHeadline: a planted PENNANT 2027 " + Tier + " Main Event win posts head:fest:title ('" + Title + "' / '" + Body + "' / " + Tag + ")");
	}
	// GRIT's two runs of one show: "-E" at 19:00, "-L" at 22:00, in the pending, its Honor and its headline.
	const ss::net::Network& Rit = ss::net::Site(Grit->Site);
	ss::net::EventInstance Early, Late;
	bool Pair = false;
	const std::vector<ss::net::EventInstance> Night = Rit.Window(FestAtWorld(Grit->FirstDay), FestAtWorld(Grit->FirstDay + 1));
	for (size_t A = 0; !Pair && A < Night.size(); ++A)
	{
		for (size_t B = 0; !Pair && B < Night.size(); ++B)
		{
			if (A != B && Night[A].Template == Night[B].Template && Night[A].Start < Night[B].Start && Rit.TemplateOf(Night[A]).Series == Grit->SeriesId)
			{
				Early = Night[A];
				Late = Night[B];
				Pair = true;
			}
		}
	}
	if (!Pair)
	{
		Expect(false, "FestHeadline: a GRIT show's two runs on " + Grit->Name + "'s first night");
		return;
	}
	world::World W = *FestSkipWorld().Skip;
	const int Who = FestPersonWorld(W, 60);
	world::World Copy = W;
	const world::Pending PE = ss::world::Sim::OnlineEvent(Copy, Early);
	const world::Pending PL = FestResolveWorld(W, Late, Who, true);
	const world::Honor* H = FestHonorWorld(W, PL.Id);
	const world::WorldEvent* Post = FestPostWorld(W, world::EventKind::Champion, PL.End, Who);
	Title = Body = Tag = std::string();
	const bool Runs = PE.Name == Rit.NameOf(Early) && PL.Name == Rit.NameOf(Late) && PE.Name.find("-E") != std::string::npos && PL.Name.find("-L") != std::string::npos;
	Expect(Runs && H && H->Title == PL.Name && Post && W.Headline(*Post, Title, Body, Tag) && Title == W.Get(Who)->Name + " wins " + PL.Name,
		"FestHeadline: a GRIT run's Pending::Name, Honor and headline read its run ('" + PE.Name + "', '" + PL.Name + "'; Honor '" + (H ? H->Title : std::string("none")) +
			"'; headline '" + Title + "')");
}

/**
 * FestTitleOrder (§2.15.1, §2.6.3): world::TitleRank's classes (1 a championship Main, 2 another festival's Main, 3 a championship belt,
 * 4 another belt, 5 a championship's Mini or Micro, 0 no title) and FestTitleBefore's order inside a class (prize, a belt's points, then
 * date): an older RCOP 2027 Main title outranks a newer, smaller PENNANT 2028 Main title; a Summer Slam Main title outranks RCOP's belt;
 * a championship belt outranks FEIGN's; the Mini and Micro rank last; an exact tie goes to the newer. FestTopTitle (the card's honorific,
 * bigfield::Honorific) picks a planted person's top one.
 * Planted bugs: ranking by date inside a class; a championship belt outranking a Summer Slam Main title.
 */
void FestTitleOrder()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* R27 = FestEditionWorld(Brand::Rcop, 2027);
	const ss::fest::Festival* P28 = FestEditionWorld(Brand::Pennant, 2028);
	const ss::fest::Festival* Slam = FestEditionWorld(Brand::Slam, 2027);
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	const ss::fest::Festival* P27 = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* G27 = FestEditionWorld(Brand::Reign, 2027);
	if (!R27 || !P28 || !Slam || !Feign || !P27 || !G27 || P27->TitleIds.size() != 3 || G27->TitleIds.size() != 3)
	{
		Expect(false, "FestTitleOrder: RCOP 2027, PENNANT 2027-2028, REIGN 2027, Summer Slam 2027 and FEIGN 2026 are in fest::All()");
		return;
	}
	auto Make = [](const std::string& Id, long long Prize, int Entries, double At) {
		world::Honor H;
		H.EventId = Id;
		H.Prize = Prize;
		H.Entries = Entries;
		H.At = At;
		H.Npc = 7;
		return H;
	};
	const world::Honor RcopMain = Make(FestMainIdWorld(*R27), 300000000, 5100, R27->MainStart + 600.0);
	const world::Honor PenMain = Make(FestMainIdWorld(*P28), 1800000, 380, P28->MainStart + 600.0);
	const world::Honor SlamMain = Make(FestMainIdWorld(*Slam), 1500000, 900, Slam->MainStart + 600.0);
	const world::Honor RcopBelt = Make(R27->SeriesId + "#pos", 0, 9000, R27->MainStart + 1440.0);
	const world::Honor FeignBelt = Make(Feign->SeriesId + "#pos", 0, 12000, Feign->MainStart + 1440.0);
	const world::Honor PenMini = Make(P27->TitleIds[1] + "@" + std::to_string(std::llround(P27->MainStart)), 500000, 1100, P27->MainStart);
	const world::Honor ReignMicro = Make(G27->TitleIds[2] + "@" + std::to_string(std::llround(G27->MainStart)), 400000, 1500, G27->MainStart);
	world::Honor Year = Make("", 0, 3000, R27->MainStart);
	Year.Title = "Player of the Year";
	const world::Honor PenMain2 = Make(FestMainIdWorld(*P28), 1800000, 380, P28->MainStart + 1200.0); // the same prize, newer
	const int Want[8] = {1, 1, 2, 3, 4, 5, 5, 0};
	const world::Honor* Ranked[8] = {&RcopMain, &PenMain, &SlamMain, &RcopBelt, &FeignBelt, &PenMini, &ReignMicro, &Year};
	std::string Ranks;
	bool Classes = true;
	for (int K = 0; K < 8; ++K)
	{
		Classes = Classes && world::TitleRank(*Ranked[K]) == Want[K];
		Ranks += " " + std::to_string(world::TitleRank(*Ranked[K]));
	}
	Expect(Classes, "FestTitleOrder: TitleRank's classes read 1 1 2 3 4 5 5 0 for RCOP and PENNANT Mains, a Summer Slam Main, RCOP's and FEIGN's belts, PENNANT's Mini, REIGN's Micro "
					"and the year's title (got" + Ranks + ")");
	const bool Order = world::FestTitleBefore(RcopMain, PenMain) && !world::FestTitleBefore(PenMain, RcopMain) && world::FestTitleBefore(SlamMain, RcopBelt) &&
					   !world::FestTitleBefore(RcopBelt, SlamMain) && world::FestTitleBefore(RcopBelt, FeignBelt) && world::FestTitleBefore(FeignBelt, PenMini) &&
					   world::FestTitleBefore(PenMini, ReignMicro) && world::FestTitleBefore(ReignMicro, Year) && !world::FestTitleBefore(Year, ReignMicro) &&
					   world::FestTitleBefore(PenMain2, PenMain) && !world::FestTitleBefore(PenMain, PenMain2);
	Expect(Order, "FestTitleOrder: class, then prize (belts by points), then date: the older, bigger RCOP Main title over the newer PENNANT one, a Summer Slam Main title over RCOP's "
				  "belt, RCOP's belt over FEIGN's, the Mini and Micro last, an exact tie to the newer");
	// FestTopTitle on a world: a person holding four kinds is introduced by the RCOP Main title; another by the Summer Slam one.
	const world::World& Mid = FestMidWorld();
	const int A = FestPersonWorld(Mid, 30);
	const int B = FestPersonWorld(Mid, A + 1);
	std::string Lines;
	for (const world::Honor* H : {&PenMain, &RcopBelt, &RcopMain, &PenMini})
	{
		Lines += FestHonorLineWorld(2027, "Planted", H->EventId, A, Mid.Get(A)->Name, H->Prize, H->Entries, H->At);
	}
	for (const world::Honor* H : {&RcopBelt, &SlamMain})
	{
		Lines += FestHonorLineWorld(2027, "Planted", H->EventId, B, Mid.Get(B)->Name, H->Prize, H->Entries, H->At);
	}
	world::World W;
	FestLoadWorld(W, FestTextWorld(Mid) + Lines);
	const world::Honor* TopA = world::FestTopTitle(W, A);
	const world::Honor* TopB = world::FestTopTitle(W, B);
	Expect(TopA && TopA->EventId == RcopMain.EventId && TopB && TopB->EventId == SlamMain.EventId && !world::FestTopTitle(W, -1),
		"FestTitleOrder: FestTopTitle introduces a person by their top title (" + (TopA ? TopA->EventId : std::string("none")) + ", " +
			(TopB ? TopB->EventId : std::string("none")) + ")");
}

/**
 * FestChampTitles' world half (§2.15.1, §2.6.2, §2.6.8; F-net's half plants the data): a planted PENNANT 2027 Mini Main Event win by a
 * person makes an Honor the roll shows on its MINI MAIN EVENT line; an unknown PENNANT 2027 Micro winner is discovered and stands on the
 * MICRO line; RCOP 2026's row (the skip world, run for real) shows its rcop-mini and rcop-micro winners, people both.
 * Planted bug: RCOP left at {MainId} (F-net's TitleIds, scratch only: F-world's half reads it).
 */
void FestChampTitlesWorld()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* Rcop = FestRcopWorld();
	ss::net::EventInstance Mini, Micro;
	if (!Pen || !Rcop || Pen->TitleIds.size() != 3 || !FestInstanceWorld(*Pen, Pen->TitleIds[1], Mini) || !FestInstanceWorld(*Pen, Pen->TitleIds[2], Micro))
	{
		Expect(false, "FestChampTitles: PENNANT 2027's three titles and their instances, RCOP 2026");
		return;
	}
	world::World W = *FestSkipWorld().Skip;
	const int Who = FestPersonWorld(W, 20);
	const world::Pending P = FestResolveWorld(W, Mini, Who);
	const size_t Before = W.People().size();
	const world::Pending Q = FestResolveWorld(W, Micro, -2);
	const std::vector<world::FestRollRow> Roll = world::FestRoll(W, Brand::Pennant);
	const bool MiniLine = Roll.size() == 1 && Roll[0].Mini && Roll[0].Mini->EventId == P.Id && Roll[0].Mini->Npc == Who && !Roll[0].Main;
	const bool MicroLine = Roll.size() == 1 && Roll[0].Micro && Roll[0].Micro->EventId == Q.Id && Roll[0].Micro->Npc == static_cast<int>(Before) && W.People().size() == Before + 1;
	Expect(MiniLine, "FestChampTitles: a planted PENNANT 2027 Mini Main Event win by a person stands on the roll's MINI MAIN EVENT line");
	Expect(MicroLine, "FestChampTitles: an unknown PENNANT 2027 Micro Main Event winner is discovered and stands on the MICRO MAIN EVENT line");
	const std::vector<world::FestRollRow> Rr = world::FestRoll(*FestSkipWorld().Skip, Brand::Rcop);
	const bool RcopRow = Rr.size() == 1 && Rr[0].Mini && Rr[0].Micro && Rr[0].Mini->Npc >= 0 && Rr[0].Micro->Npc >= 0 && !Rr[0].Mini->Name.empty() && !Rr[0].Micro->Name.empty() &&
						 FestStartsWorld(Rr[0].Mini->EventId, "rcop-mini@") && FestStartsWorld(Rr[0].Micro->EventId, "rcop-micro@");
	Expect(RcopRow, "FestChampTitles: RCOP 2026's row shows its rcop-mini and rcop-micro winners (" + (Rr.empty() || !Rr[0].Mini ? std::string("no Mini") : Rr[0].Mini->Name) + ", " +
						(Rr.empty() || !Rr[0].Micro ? std::string("no Micro") : Rr[0].Micro->Name) + ")");
}

/**
 * FestMajorFloor's world half (§2.15.1, §2.6.2; F-net's half holds T.Main by the floor): PENNANT 2027's Micro Main Event (a title, no
 * major) won by a person makes an Honor (Lasting's FestivalTitle term) and leaves their majors where they were; won by the hero, its Honor
 * leaves World::HallScore unchanged, while the hero's PENNANT 2027 Mini Main Event (a major) adds a major's 3.0.
 * Planted bug: HallScore counting a non-major title Honor.
 */
void FestMajorFloorWorld()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	ss::net::EventInstance Mini, Micro;
	if (!Pen || Pen->TitleIds.size() != 3 || !FestInstanceWorld(*Pen, Pen->TitleIds[1], Mini) || !FestInstanceWorld(*Pen, Pen->TitleIds[2], Micro))
	{
		Expect(false, "FestMajorFloor: PENNANT 2027's Mini and Micro Main Events and their instances");
		return;
	}
	world::World W = *FestSkipWorld().Skip;
	const int Who = FestPersonWorld(W, 25);
	const int Majors = W.Get(Who)->Majors;
	const world::Pending P = FestResolveWorld(W, Micro, Who);
	const world::Honor* H = FestHonorWorld(W, P.Id);
	Expect(!P.Major && H && H->Npc == Who && W.Get(Who)->Majors == Majors,
		"FestMajorFloor: a person's PENNANT 2027 Micro Main Event win (no major) keeps its Honor and adds no major (" + std::string(P.Major ? "a major" : "no major") + ", Honor " +
			(H ? "kept" : "missing") + ", majors " + std::to_string(Majors) + " -> " + std::to_string(W.Get(Who)->Majors) + ")");
	world::World Hero = *FestSkipWorld().Skip;
	const double Before = Hero.HallScore(-1, 2027);
	const world::Pending Q = FestResolveWorld(Hero, Micro, -1);
	const world::Honor* HeroH = FestHonorWorld(Hero, Q.Id);
	const double Minor = Hero.HallScore(-1, 2027);
	const world::Pending M = FestResolveWorld(Hero, Mini, -1);
	const double Major = Hero.HallScore(-1, 2027);
	Expect(HeroH && HeroH->Npc == -1 && Before == Minor && M.Major && std::fabs(Major - Minor - 3.0) < 1e-9,
		"FestMajorFloor: the hero's Micro Main Event title keeps its Honor and HallScore skips it (" + std::to_string(Before) + " -> " + std::to_string(Minor) +
			"); the Mini Main Event, a major, adds 3.0 (-> " + std::to_string(Major) + ")");
}

// ------------------------------------------------------------------ FW-4: WHO'S IN (§4.7.3, §2.6.6, §2.6.7, §2.8.4; R-16, R-21).
// Honors, a bond and three queued fields are planted through the world text on the day-50 world; nothing is simulated past it (§8.1).

/** Text escaped as the save writes a field (backslash, tab, newline, '|' and '~'). */
std::string FestEscWorld(const std::string& S)
{
	std::string Out;
	for (const char C : S)
	{
		Out += C == '\\' ? std::string("\\\\")
			: C == '\t'  ? std::string("\\t")
			: C == '\n'  ? std::string("\\n")
			: C == '|'   ? std::string("\\p")
			: C == '~'   ? std::string("\\s")
						 : std::string(1, C);
	}
	return Out;
}

/** A number as text that reads back exactly (%.17g). */
std::string FestNumWorld(double V)
{
	char Buf[40];
	std::snprintf(Buf, sizeof(Buf), "%.17g", V);
	return Buf;
}

/** A world\tq line as the save writes a queued event: its fields in World::Write's order, then its registrations as
 *  npc~ticket~staked~bullets~share~known~better records. */
std::string FestQueueLineWorld(const ss::world::Pending& P)
{
	const auto Int = [](long long V) { return std::to_string(V); };
	std::string Who;
	for (const ss::world::Entry& E : P.Who)
	{
		Who += (Who.empty() ? "" : "|") + Int(E.Npc) + "~" + Int(E.Ticket ? 1 : 0) + "~" + Int(E.Staked ? 1 : 0) + "~" + Int(E.Bullets) + "~" + FestNumWorld(E.Share) +
			   "~" + Int(E.Known) + "~" + Int(E.Better);
	}
	const std::string Fields[] = {FestEscWorld(P.Id), FestEscWorld(P.Name), Int(P.Online ? 1 : 0), Int(P.Template), Int(P.Kind), FestNumWorld(P.Start),
		FestNumWorld(P.End), Int(P.Entries), Int(P.Pool), Int(P.BuyIn), Int(P.Tier), Int(P.Format), Int(P.TableSize), Int(P.Major ? 1 : 0), Int(P.Bracelet ? 1 : 0),
		Int(P.Ring ? 1 : 0), FestEscWorld(P.Series), FestEscWorld(P.Ticket), Int(P.SeatValue), Int(P.Stakes), Int(P.HeroPlace), Int(P.HeroPrize), FestNumWorld(P.Strength),
		FestNumWorld(P.Luck), Int(P.Kinds), Int(P.Seats), Int(P.FinalSize), Who};
	std::string Out = "world\tq";
	for (const std::string& F : Fields)
	{
		Out += "\t" + F;
	}
	return Out + "\n";
}

/** The belt Honor's points planted for RCOP 2026 (§2.6.7's {Points}). */
constexpr int kFestBeltPointsWorld = 4321;

/** WHO'S IN's planted world (FestStoriesWorld) and the people and fields it holds. */
struct FestStoriesRunWorld
{
	std::unique_ptr<ss::world::World> W;
	bool Found = false; // the editions, the instances and the people exist
	std::string Missing;
	std::string MainId, MiniId, OtherId, IdleId; // RCOP 2027's Main and Mini Main Event, a field that is no festival's, an unqueued event
	int Rival = -1, MetP = -1, Def = -1, Belt = -1, Year = -1, Pro = -1, Streamer = -1, Qual = -1, Nobody = -1, Late = -1, Grinder = -1;
	double Ms = 0.0; // the build: copy, plant, write, read
};

/**
 * The skip run's day-50 world (RCOP 2026 crowned; Team RiverLine signed its first class on Nov 1) with, planted through its text: Honors
 * first in history, so they stand on the roll before the ones RCOP 2026 made (RCOP 2026's Main to Def, its Mini and Micro Main Events
 * to Pro, its belt to Belt with 4,321 points, PENNANT 2027's and REIGN 2027's Mains to Year, FEIGN 2026's Main to Streamer), a bond with
 * a memory for MetP, and three queued fields as world\tq lines: RCOP 2027's Main (ten registrations, Def twice; tickets for Qual, Year
 * and the rival), its Mini Main Event (Def not registered; a ticket for Year; Grinder, a plain person with two or more wins among their
 * Recent finishes and no Honor) and a RiverLine event on day 75 that is no festival's (a ticket for Qual). Rival, Pro (a team pro),
 * Streamer and Late (Kast streamers) carry their flags; the rest are people with none; nobody picked but the rival holds a festival title
 * or a bond of their own. Built once.
 */
FestStoriesRunWorld& FestStoriesWorld()
{
	namespace world = ss::world;
	using ss::fest::Brand;
	static FestStoriesRunWorld Run;
	if (Run.W)
	{
		return Run;
	}
	const auto T0 = std::chrono::steady_clock::now();
	Run.W = std::make_unique<world::World>();
	world::World Base = *FestSkipWorld().Skip;
	const ss::fest::Festival* R26 = FestRcopWorld();
	const ss::fest::Festival* R27 = FestEditionWorld(Brand::Rcop, 2027);
	const ss::fest::Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* Reign = FestEditionWorld(Brand::Reign, 2027);
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	ss::net::EventInstance Main, Mini, Idle, Mini26, Micro26, Other;
	const bool Editions = R26 && R27 && Pen && Reign && Feign && R26->TitleIds.size() == 3 && R27->TitleIds.size() == 3 && FestInstanceWorld(*R27, R27->MainId, Main) &&
						  FestInstanceWorld(*R27, R27->TitleIds[1], Mini) && FestInstanceWorld(*R27, R27->TitleIds[2], Idle) &&
						  FestInstanceWorld(*R26, R26->TitleIds[1], Mini26) && FestInstanceWorld(*R26, R26->TitleIds[2], Micro26);
	const ss::net::Network& N = ss::net::Shared();
	bool HaveOther = false;
	for (const ss::net::EventInstance& E : N.Window(FestAtWorld(75), FestAtWorld(76)))
	{
		const ss::net::EventTemplate& T = N.TemplateOf(E);
		if (!HaveOther && T.Series.empty() && T.Fmt != ss::net::Format::Satellite)
		{
			Other = E;
			HaveOther = true;
		}
	}
	std::vector<int> Titled; // anyone the skip run gave a festival title or belt
	for (const world::Honor& H : Base.Honors())
	{
		if (world::TitleRank(H) > 0)
		{
			Titled.push_back(H.Npc);
		}
	}
	std::sort(Titled.begin(), Titled.end());
	std::vector<int> Plain;
	for (int K = 1; K < static_cast<int>(Base.People().size()); ++K)
	{
		const world::Npc* P = Base.Get(K);
		if (!P || P->Faded || (!P->Rival && (std::binary_search(Titled.begin(), Titled.end(), K) || Base.Bonds().count(K) > 0)))
		{
			continue;
		}
		if (P->Rival)
		{
			Run.Rival = Run.Rival < 0 ? K : Run.Rival;
		}
		else if (P->Pro)
		{
			Run.Pro = Run.Pro < 0 ? K : Run.Pro;
		}
		else if (P->Streams && !P->Pro && Run.Streamer < 0)
		{
			Run.Streamer = K;
		}
		else if (P->Streams && !P->Pro && Run.Late < 0)
		{
			Run.Late = K;
		}
		else if (!P->Pro && !P->Streams && Plain.size() < 6)
		{
			Plain.push_back(K);
		}
		else if (!P->Pro && !P->Streams && Run.Grinder < 0 &&
				 std::count_if(P->Recent.begin(), P->Recent.end(), [](const world::Finish& F) { return F.Place == 1; }) >= 2)
		{
			Run.Grinder = K; // two or more Recent wins and no Honor: no title holder (titles are counted from Honors, §2.6.7)
		}
	}
	Run.Found = Editions && HaveOther && Run.Rival >= 0 && Run.Pro >= 0 && Run.Streamer >= 0 && Run.Late >= 0 && Plain.size() == 6 && Run.Grinder >= 0;
	if (!Run.Found)
	{
		Run.Missing = std::string(Editions ? "" : "editions or instances; ") + (HaveOther ? "" : "a day-75 RiverLine event with no series; ") +
					  "rival " + std::to_string(Run.Rival) + ", pro " + std::to_string(Run.Pro) + ", streamers " + std::to_string(Run.Streamer) + "/" +
					  std::to_string(Run.Late) + ", plain people " + std::to_string(Plain.size()) + ", grinder " + std::to_string(Run.Grinder);
		return Run;
	}
	Run.Def = Plain[0];
	Run.Belt = Plain[1];
	Run.Year = Plain[2];
	Run.MetP = Plain[3];
	Run.Qual = Plain[4];
	Run.Nobody = Plain[5];
	Run.MainId = Main.Id;
	Run.MiniId = Mini.Id;
	Run.OtherId = Other.Id;
	Run.IdleId = Idle.Id;
	Base.Remember(Run.MetP, world::MemoryKind::Met, N.TemplateOf(Other).Name, 0, Base.Clock());
	const auto Name = [&Base](int Id) { return Base.Get(Id)->Name; };
	const std::string Honors =
		FestHonorLineWorld(2026, FestTemplateNameWorld(*R26, R26->MainId), FestMainIdWorld(*R26), Run.Def, Name(Run.Def), 250000000, 4100, R26->MainStart + 600.0) +
		FestHonorLineWorld(2026, FestTemplateNameWorld(*R26, R26->TitleIds[1]), Mini26.Id, Run.Pro, Name(Run.Pro), 4800000, Mini26.Entries, Mini26.Start + 600.0) +
		FestHonorLineWorld(2026, FestTemplateNameWorld(*R26, R26->TitleIds[2]), Micro26.Id, Run.Pro, Name(Run.Pro), 900000, Micro26.Entries, Micro26.Start + 600.0) +
		FestHonorLineWorld(2026, R26->Name + " Player of the Series", R26->SeriesId + "#pos", Run.Belt, Name(Run.Belt), 0, kFestBeltPointsWorld,
			FestHourAfterWorld(ss::fest::LastEnd(*R26))) +
		FestHonorLineWorld(2027, FestTemplateNameWorld(*Pen, Pen->MainId), FestMainIdWorld(*Pen), Run.Year, Name(Run.Year), 30000000, 2400, Pen->MainStart + 600.0) +
		FestHonorLineWorld(2027, FestTemplateNameWorld(*Reign, Reign->MainId), FestMainIdWorld(*Reign), Run.Year, Name(Run.Year), 40000000, 2600, Reign->MainStart + 600.0) +
		FestHonorLineWorld(2026, FestTemplateNameWorld(*Feign, Feign->MainId), FestMainIdWorld(*Feign), Run.Streamer, Name(Run.Streamer), 20000000, 1800, Feign->MainStart + 600.0);
	const auto Queued = [&Base](const ss::net::EventInstance& E, const std::vector<std::pair<int, bool>>& Who) {
		world::Pending P = world::Sim::OnlineEvent(Base, E);
		P.Who.clear();
		P.HeroPlace = 0;
		P.HeroPrize = 0;
		for (const std::pair<int, bool>& In : Who)
		{
			world::Entry X;
			X.Npc = In.first;
			X.Ticket = In.second;
			P.Who.push_back(X);
		}
		return FestQueueLineWorld(P);
	};
	const std::string Queue =
		Queued(Main, {{Run.Nobody, false}, {Run.Qual, true}, {Run.Streamer, false}, {Run.Year, true}, {Run.Pro, false}, {Run.Belt, false}, {Run.Def, false},
						 {Run.MetP, false}, {Run.Rival, true}, {Run.Def, false}}) +
		Queued(Mini, {{Run.Belt, false}, {Run.Pro, false}, {Run.Year, true}, {Run.Nobody, false}, {Run.Streamer, false}, {Run.Grinder, false}}) +
		Queued(Other, {{Run.Pro, false}, {Run.Streamer, false}, {Run.Def, false}, {Run.Belt, false}, {Run.Year, false}, {Run.Nobody, false}, {Run.Rival, false},
						  {Run.Qual, true}});
	std::string Text = FestPlantFirstWorld(FestTextWorld(Base), Honors);
	const size_t FirstQ = Text.find("world\tq\t");
	Text.insert(FirstQ == std::string::npos ? Text.size() : FirstQ, Queue);
	FestLoadWorld(*Run.W, Text);
	Run.Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	return Run;
}

/** A StoryWhy's name (the checks' messages). */
const char* FestWhyNameWorld(ss::world::StoryWhy Why)
{
	const char* const Names[] = {"Rival", "Following", "Met", "Wake", "DefendingChampion", "DefendingSeries", "Champion", "YearChampion", "TitleHolder", "TeamPro",
		"Streamer", "Rising", "Qualifier"};
	const size_t K = static_cast<size_t>(Why);
	return K < sizeof(Names) / sizeof(Names[0]) ? Names[K] : "none";
}

/** People as "npc Why" items, in order (the checks compare these). */
std::string FestPeopleTextWorld(const std::vector<std::pair<int, ss::world::StoryWhy>>& People)
{
	std::string Out;
	for (const std::pair<int, ss::world::StoryWhy>& P : People)
	{
		Out += (Out.empty() ? "" : ", ") + std::to_string(P.first) + " " + FestWhyNameWorld(P.second);
	}
	return Out;
}

std::string FestFieldTextWorld(const ss::world::FieldStories& F)
{
	std::vector<std::pair<int, ss::world::StoryWhy>> People;
	for (const ss::world::StoryPerson& P : F.People)
	{
		People.emplace_back(P.Npc, P.Why);
	}
	return FestPeopleTextWorld(People);
}

/** A field's group counts, in StoryGroup's order. */
std::string FestGroupsWorld(const ss::world::FieldStories& F)
{
	std::string Out;
	for (const int G : F.Groups)
	{
		Out += (Out.empty() ? "" : " ") + std::to_string(G);
	}
	return Out;
}

/** The reason Npc is listed under in EventId's field (StoryWhy::Count: not listed). */
ss::world::StoryWhy FestListedWorld(const ss::world::World& W, const std::string& EventId, int Npc)
{
	for (const ss::world::StoryPerson& P : W.FieldStoriesOf(EventId).People)
	{
		if (P.Npc == Npc)
		{
			return P.Why;
		}
	}
	return ss::world::StoryWhy::Count;
}

/**
 * FestFieldStories (§2.15.1, §4.7.3, §2.8.4): WHO'S IN over a field's queued registrations, each person once under the first reason in
 * StoryWhy's order (registration order within one). RCOP 2027's Main lists the rival, the person met, RCOP 2026's Main champion (the
 * defending champion, registered), its belt holder (the defending Player of the Series), the year's other champion, the two-title holder,
 * the streamer and the qualifier, and nobody without a story; its Qualifiers group counts its three tickets. On its Mini Main Event the
 * defender, not registered, is not listed (nobody is DefendingChampion) and the belt holder is a champion on the roll. In a field that is
 * no festival's the Main and belt holders of any brand are champions and nobody defends. An event not queued has no stories. Kept per
 * world state: one object for two reads with other events read between; a copy reads the same; a registration (World::Join: the revision
 * moves) shows at once.
 * Planted bugs: an unregistered defender listed (the SPEC's); a person listed twice; registration order kept; the field kept across a
 * revision.
 */
void FestFieldStories()
{
	namespace world = ss::world;
	using Why = world::StoryWhy;
	FestStoriesRunWorld& Run = FestStoriesWorld();
	if (!Run.Found)
	{
		Expect(false, "FestFieldStories: the planted world's editions, instances and people exist (" + Run.Missing + ")");
		return;
	}
	const world::World& W = *Run.W;
	const size_t Sizes[] = {W.RegisteredNow(Run.MainId).size(), W.RegisteredNow(Run.MiniId).size(), W.RegisteredNow(Run.OtherId).size()};
	Expect(Sizes[0] == 10 && Sizes[1] == 6 && Sizes[2] == 8, "FestFieldStories: the three planted fields are queued as written (" + std::to_string(Sizes[0]) + ", " +
																  std::to_string(Sizes[1]) + ", " + std::to_string(Sizes[2]) + " registrations)");
	const std::string WantMain = FestPeopleTextWorld({{Run.Rival, Why::Rival}, {Run.MetP, Why::Met}, {Run.Def, Why::DefendingChampion}, {Run.Belt, Why::DefendingSeries},
		{Run.Year, Why::YearChampion}, {Run.Pro, Why::TitleHolder}, {Run.Streamer, Why::Streamer}, {Run.Qual, Why::Qualifier}});
	const world::FieldStories& Main = W.FieldStoriesOf(Run.MainId);
	Expect(FestFieldTextWorld(Main) == WantMain && FestGroupsWorld(Main) == "3 0 0 0 0",
		"FestFieldStories: RCOP 2027's Main lists each person once in StoryWhy's order, the registered defender among them, and counts 3 qualifiers (got [" +
			FestFieldTextWorld(Main) + "] groups " + FestGroupsWorld(Main) + "; want [" + WantMain + "] groups 3 0 0 0 0)");
	const std::string WantMini = FestPeopleTextWorld({{Run.Belt, Why::Champion}, {Run.Pro, Why::TitleHolder}, {Run.Streamer, Why::Streamer}, {Run.Year, Why::Qualifier}});
	const world::FieldStories& Mini = W.FieldStoriesOf(Run.MiniId);
	bool Defends = false;
	for (const world::StoryPerson& P : Mini.People)
	{
		Defends = Defends || P.Npc == Run.Def || P.Why == Why::DefendingChampion;
	}
	Expect(!Defends && FestFieldTextWorld(Mini) == WantMini && FestGroupsWorld(Mini) == "1 0 0 0 0",
		"FestFieldStories: on RCOP 2027's Mini Main Event the defender, not registered, is not listed and the belt holder is a champion on the roll (got [" +
			FestFieldTextWorld(Mini) + "] groups " + FestGroupsWorld(Mini) + "; want [" + WantMini + "])");
	const std::string WantOther = FestPeopleTextWorld({{Run.Rival, Why::Rival}, {Run.Streamer, Why::Champion}, {Run.Def, Why::Champion}, {Run.Belt, Why::Champion},
		{Run.Year, Why::Champion}, {Run.Pro, Why::TeamPro}, {Run.Qual, Why::Qualifier}});
	const world::FieldStories& Other = W.FieldStoriesOf(Run.OtherId);
	const world::FieldStories& Idle = W.FieldStoriesOf(Run.IdleId);
	Expect(FestFieldTextWorld(Other) == WantOther && FestGroupsWorld(Other) == "1 0 0 0 0" && Idle.People.empty() && FestGroupsWorld(Idle) == "0 0 0 0 0",
		"FestFieldStories: in a field that is no festival's anyone with a festival Main or a belt (any brand) is a champion and nobody defends; an unqueued event has "
		"no stories (got [" +
			FestFieldTextWorld(Other) + "]; want [" + WantOther + "]; unqueued: " + std::to_string(Idle.People.size()) + " people)");
	// Kept per world state: one object across reads of other events; a copy (its own views) reads the same; a registration shows at once.
	const world::FieldStories* First = &W.FieldStoriesOf(Run.MainId);
	(void)W.FieldStoriesOf(Run.MiniId);
	(void)W.FieldStoriesOf(Run.OtherId);
	(void)W.FieldStoriesOf(Run.IdleId);
	const bool Kept = First == &Main && First == &W.FieldStoriesOf(Run.MainId) && FestFieldTextWorld(*First) == WantMain;
	world::World Copy = W;
	const auto T0 = std::chrono::steady_clock::now();
	const std::string CopyText = FestFieldTextWorld(Copy.FieldStoriesOf(Run.MainId));
	const auto T1 = std::chrono::steady_clock::now();
	(void)Copy.FieldStoriesOf(Run.MainId);
	const auto T2 = std::chrono::steady_clock::now();
	Copy.Join(Run.MainId, Run.Late);
	const std::string Joined = FestFieldTextWorld(Copy.FieldStoriesOf(Run.MainId));
	const std::string WantJoined = FestPeopleTextWorld({{Run.Rival, Why::Rival}, {Run.MetP, Why::Met}, {Run.Def, Why::DefendingChampion},
		{Run.Belt, Why::DefendingSeries}, {Run.Year, Why::YearChampion}, {Run.Pro, Why::TitleHolder}, {Run.Streamer, Why::Streamer}, {Run.Late, Why::Streamer},
		{Run.Qual, Why::Qualifier}});
	Expect(Kept && CopyText == WantMain && Joined == WantJoined,
		std::string("FestFieldStories: kept per world state (one object across reads of other events: ") + (Kept ? "yes" : "no") +
			"); a copy reads the same; a registration (World::Join) shows at once (got [" + Joined + "]; want [" + WantJoined + "])");
	std::printf("  FestFieldStories: the planted world built in %.0f ms; RCOP 2027's Main walked in %.3f ms (10 registrations), a cached read %.4f ms\n", Run.Ms,
		std::chrono::duration<double, std::milli>(T1 - T0).count(), std::chrono::duration<double, std::milli>(T2 - T1).count());
}

/**
 * FestStoryHooks (§2.15.1, §2.6.7, §2.11; R-16, R-21): the championships' three storylines in WHO'S IN, from Honors planted through the
 * world text (never simulated to day 554, §8.1). On RCOP 2027's Main RCOP 2026's belt holder is the defending Player of the Series,
 * "DEFENDING PLAYER OF THE SERIES · {Name} · 4,321 pts" (the belt Honor's points); the holder of PENNANT 2027's and REIGN 2027's Mains is
 * the year's champion, "{Name} · REIGN 2027 champion" (the latest); the holder of RCOP 2026's Mini and Micro Main Event titles (Honors
 * only: no RCOP title among their Recent finishes) holds two RCOP titles, "{Name} · 2 RCOP titles", and a registrant with two or more Recent
 * wins but no Honor holds none. On the Mini Main Event (a title event, not
 * the Main) only the title count holds; in a field that is no festival's none does. FestWhyLine answers "" for every other reason (R-21:
 * their labels are F-hubs', U38) and wherever a reason's fact doesn't hold. The ChampionshipPair beat is W-story's at G2.
 * Planted bugs: a title count taken from Recent instead of Honors (the SPEC's); the defending Player of the Series on every championship
 * event; FestWhyLine widened to the Champion reason.
 */
void FestStoryHooks()
{
	namespace world = ss::world;
	using Why = world::StoryWhy;
	FestStoriesRunWorld& Run = FestStoriesWorld();
	if (!Run.Found)
	{
		Expect(false, "FestStoryHooks: the planted world's editions, instances and people exist (" + Run.Missing + ")");
		return;
	}
	const world::World& W = *Run.W;
	const auto Line = [&W](int Npc, Why Y, const std::string& EventId) {
		world::StoryPerson P;
		P.Npc = Npc;
		P.Why = Y;
		return world::FestWhyLine(W, P, EventId);
	};
	const auto NameOf = [&W](int Npc) { return W.Get(Npc)->Name; };
	const std::string Dot = " \xC2\xB7 ";
	// The defending Player of the Series: the championship's Main only (§2.6.7: "WHO'S IN on the championship Main").
	const std::string Series = Line(Run.Belt, Why::DefendingSeries, Run.MainId);
	const std::string WantSeries = "DEFENDING PLAYER OF THE SERIES" + Dot + NameOf(Run.Belt) + Dot + "4,321 pts";
	Expect(FestListedWorld(W, Run.MainId, Run.Belt) == Why::DefendingSeries && Series == WantSeries && FestListedWorld(W, Run.MiniId, Run.Belt) == Why::Champion &&
			   Line(Run.Belt, Why::DefendingSeries, Run.MiniId).empty(),
		"FestStoryHooks: RCOP 2026's belt holder is the defending Player of the Series on RCOP 2027's Main only, with the belt Honor's points (\"" + Series +
			"\"; on the Mini Main Event: " + FestWhyNameWorld(FestListedWorld(W, Run.MiniId, Run.Belt)) + ")");
	// The year's chase: another championship's Main title from the same year, on a championship's Main; the line names the latest.
	const std::string Year = Line(Run.Year, Why::YearChampion, Run.MainId);
	const std::string WantYear = NameOf(Run.Year) + Dot + "REIGN 2027 champion";
	Expect(FestListedWorld(W, Run.MainId, Run.Year) == Why::YearChampion && Year == WantYear && FestListedWorld(W, Run.MiniId, Run.Year) == Why::Qualifier &&
			   FestListedWorld(W, Run.OtherId, Run.Year) == Why::Champion && Line(Run.Year, Why::YearChampion, Run.MiniId).empty() &&
			   Line(Run.Year, Why::YearChampion, Run.OtherId).empty(),
		"FestStoryHooks: PENNANT 2027's and REIGN 2027's Main champion is the year's champion on RCOP 2027's Main, the line naming the latest (\"" + Year +
			"\"), and nowhere else");
	// The repeat-title watch: two of the brand's titles (any of the trio, any edition), counted from Honors, on any title event.
	bool RecentWin = false; // an RCOP title among the title holder's Recent finishes (a win at an event named as an RCOP title)
	for (const world::Finish& F : W.Get(Run.Pro)->Recent)
	{
		for (const ss::fest::Festival& E : ss::fest::All())
		{
			for (const std::string& Id : E.TitleIds)
			{
				RecentWin = RecentWin || (E.Which == ss::fest::Brand::Rcop && F.Place == 1 && F.Event == FestTemplateNameWorld(E, Id));
			}
		}
	}
	const std::vector<world::Finish>& Grind = W.Get(Run.Grinder)->Recent; // two or more wins, no Honor: never a title holder
	const long long GrindWins = std::count_if(Grind.begin(), Grind.end(), [](const world::Finish& F) { return F.Place == 1; });
	const std::string Titles = Line(Run.Pro, Why::TitleHolder, Run.MainId);
	const std::string WantTitles = NameOf(Run.Pro) + Dot + "2 RCOP titles";
	Expect(!RecentWin && GrindWins >= 2 && FestListedWorld(W, Run.MiniId, Run.Grinder) == Why::Count && FestListedWorld(W, Run.MainId, Run.Pro) == Why::TitleHolder && FestListedWorld(W, Run.MiniId, Run.Pro) == Why::TitleHolder &&
			   FestListedWorld(W, Run.OtherId, Run.Pro) == Why::TeamPro && Titles == WantTitles && Line(Run.Pro, Why::TitleHolder, Run.MiniId) == WantTitles &&
			   Line(Run.Pro, Why::TitleHolder, Run.OtherId).empty(),
		"FestStoryHooks: the holder of RCOP 2026's Mini and Micro Main Event titles (from Honors; an RCOP title in their Recent finishes: " + std::string(RecentWin ? "one" : "none") +
			") holds 2 RCOP titles on its title events (\"" + Titles + "\"), a team pro elsewhere; a registrant with " + std::to_string(GrindWins) +
			" Recent wins and no Honor is listed as " + FestWhyNameWorld(FestListedWorld(W, Run.MiniId, Run.Grinder)));
	// Every other reason prints no festival line (R-16, R-21); the three lines leave no placeholder.
	std::string Loud;
	for (const std::string* Id : {&Run.MainId, &Run.MiniId, &Run.OtherId})
	{
		for (const world::StoryPerson& P : W.FieldStoriesOf(*Id).People)
		{
			const bool Hook = P.Why == Why::DefendingSeries || P.Why == Why::YearChampion || P.Why == Why::TitleHolder;
			const std::string L = world::FestWhyLine(W, P, *Id);
			if (Hook == L.empty())
			{
				Loud += (Loud.empty() ? "" : "; ") + std::to_string(P.Npc) + " " + FestWhyNameWorld(P.Why) + " \"" + L + "\"";
			}
		}
	}
	const bool Filled = (Series + Year + Titles).find('{') == std::string::npos;
	Expect(Loud.empty() && Filled, "FestStoryHooks: only the three championship reasons print a festival line, every placeholder filled (" +
									   (Loud.empty() ? std::string("none other") : Loud) + ")");
	std::printf("  FestStoryHooks: \"%s\" | \"%s\" | \"%s\"\n", Series.c_str(), Year.c_str(), Titles.c_str());
}

// ------------------------------------------------------------------ FW-5: the session's texts, moments and Rerun (§2.8.5, §2.9.3, §2.9.4)

/** Every text a session sends, and its last save. */
struct FestHooksWorld : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Texts;
	ss::SaveData Last;
	int Saves = 0;
	void Text(const std::string& From, const std::string& Body) override { Texts.emplace_back(From, Body); }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

/** The festival texts among Hooks' texts from index From on (Dee's belt and mark lines, T10 and T14), "|"-joined. */
std::string FestDeeTextsWorld(const FestHooksWorld& H, size_t From)
{
	std::string Out;
	for (size_t I = From; I < H.Texts.size(); ++I)
	{
		const std::string& B = H.Texts[I].second;
		if (H.Texts[I].first == "Dee" && (B.rfind("There's a belt", 0) == 0 || B.find(" in the race. I'm telling people.") != std::string::npos))
		{
			Out += (Out.empty() ? "" : "|") + B;
		}
	}
	return Out;
}

/** A moment as one line ("2 rcop 1700 m5 1/608", "-" for none). */
std::string FestMomentLineWorld(const std::optional<ss::FestMoment>& M)
{
	if (!M)
	{
		return "-";
	}
	return std::to_string(M->Kind) + " " + M->Key + " " + std::to_string(M->Prize) + " m" + std::to_string(M->Mark) + " " + std::to_string(M->Rank) + "/" +
		std::to_string(M->Scorers) + (M->Dev ? " dev" : "");
}

/** A festival story candidate's state in a session's channel: "-" when it isn't open, else its packed facts (and " series"). */
std::string FestStoryStateWorld(const ss::Session& S, const std::string& Key)
{
	for (const ss::video::Candidate& C : S.Life.Video.Open)
	{
		if (C.Id == "story:" + Key)
		{
			return C.IdeaFacts + (C.Series ? " series" : "");
		}
	}
	return "-";
}

/** One mark planted into a session's world as its save line reads (world\tfest\tmark: key, grade, rank, scorers, points). */
void FestPlantMarkWorld(ss::world::World& W, const std::string& Key, ss::world::FestMark Mark, int Rank, int Scorers, int Points)
{
	W.Read({"world", "fest", "mark", Key, std::to_string(static_cast<int>(Mark)), std::to_string(Rank), std::to_string(Scorers), std::to_string(Points)});
}

/** The session half of FestTexts and FestMoments, run once: a fresh career whose save already holds the Championship Online 2027 Main's
 *  final-table line, three marks planted (RCOP 2026's belt, Ring Rush 2027's PODIUM, the Championship Online 2027's FINALIST), two
 *  skips, a step, the belt's moment dismissed, a reload, and the rest dismissed; Rerun's channel opened before the marks (the old final
 *  table's story marked told), and two calendar steps after the skip and after the reload (Rerun notes the belt's story). */
struct FestSessionRunWorld
{
	std::string Skip1, Again, Reloaded;              // the festival texts each stretch sent
	size_t MarksBack = 0;                            // the marks the reloaded world holds
	std::string Peek1, Peek2, Peek3, Peek4, Peek5;   // the moments in turn (1 and 2 before the reload)
	bool BeltKeyKept = false, MarkKeyKept = false;   // the done keys in the last save
	std::string Story, OldStory, StoryLoaded, StoryBack; // Rerun's belt story (noted, reloaded, stepped) and the old final table's
	bool Parsed = false;                                 // the save's text read back
	double Ms = 0.0;
};
FestSessionRunWorld& FestSessionWorld()
{
	static std::unique_ptr<FestSessionRunWorld> Run;
	if (Run)
	{
		return *Run;
	}
	Run = std::make_unique<FestSessionRunWorld>();
	namespace world = ss::world;
	const auto T0 = std::chrono::steady_clock::now();
	FestHooksWorld H0;
	ss::SaveData D;
	{
		ss::Session S0(H0, "festtexts");
		S0.WorldSkip(1);
		D = H0.Last;
	}
	D.TextsSeen.push_back("fest-ft-tco27"); // the edition's win-or-final-table line already went (the cap's second slot)
	FestHooksWorld H1;
	{
		ss::Session S(H1, "festtexts", &D);
		FestPlantMarkWorld(S.Living(), "rcop", world::FestMark::Title, 1, 608, 1700);
		FestPlantMarkWorld(S.Living(), "ring27", world::FestMark::Podium, 2, 500, 900);
		FestPlantMarkWorld(S.Living(), "tco27", world::FestMark::Finalist, 7, 1234, 600);
		S.DebugVideo("fake win online"); // the channel's first state: the stories already told are marked (the old final table's)
		size_t From = H1.Texts.size();
		S.WorldSkip(1); // WorldSkip's FestStep: nothing else steps the planted marks
		Run->Skip1 = FestDeeTextsWorld(H1, From);
		From = H1.Texts.size();
		S.Update(1000.0);
		S.Update(1001.0); // a calendar step: Rerun notes the belt's story
		Run->Story = FestStoryStateWorld(S, "fest-belt-rcop");
		Run->OldStory = FestStoryStateWorld(S, "fest-ft-tco27");
		S.WorldSkip(1);
		S.FestStep(S.WorldMinutes(), S.WorldMinutes() + 60.0);
		Run->Again = FestDeeTextsWorld(H1, From);
		Run->Peek1 = FestMomentLineWorld(S.PeekFestMoment());
		S.DoneFestMoment();
		Run->Peek2 = FestMomentLineWorld(S.PeekFestMoment()); // not dismissed: the reload plays it again
	}
	FestHooksWorld H2;
	ss::SaveData Back; // the save's own text round trip (what a reload from disk reads)
	Run->Parsed = ss::SaveData::Parse(H1.Last.Serialize(), Back);
	{
		ss::Session S2(H2, "festtexts", &Back);
		Run->MarksBack = world::HeroMarks(S2.Living()).size();
		Run->StoryLoaded = FestStoryStateWorld(S2, "fest-belt-rcop");
		const size_t From = H2.Texts.size();
		S2.FestStep(S2.WorldMinutes(), S2.WorldMinutes() + 60.0);
		S2.Update(1000.0);
		S2.Update(1001.0);
		Run->StoryBack = FestStoryStateWorld(S2, "fest-belt-rcop");
		S2.WorldSkip(1);
		Run->Reloaded = FestDeeTextsWorld(H2, From);
		Run->Peek3 = FestMomentLineWorld(S2.PeekFestMoment());
		S2.DoneFestMoment();
		Run->Peek4 = FestMomentLineWorld(S2.PeekFestMoment());
		S2.DoneFestMoment();
		Run->Peek5 = FestMomentLineWorld(S2.PeekFestMoment());
	}
	const std::set<std::string> Done = H2.Last.Life.Sites.Done;
	Run->BeltKeyKept = Done.count("festbelt:rcop") > 0;
	Run->MarkKeyKept = Done.count("festmark:ring27") > 0 && Done.count("festmark:tco27") > 0;
	Run->Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	return *Run;
}

void FestTexts()
{
	namespace sfd = ss::session_fest_detail;
	using ss::fest::Brand;
	using ss::fest::Festival;
	const Festival* Rcop = FestRcopWorld();
	const Festival* Tco = FestEditionWorld(Brand::Tco, 2027);
	const Festival* Pen = FestEditionWorld(Brand::Pennant, 2027);
	const Festival* Feign = FestEditionWorld(Brand::Feign, 2026);
	ss::net::EventInstance Numbered;
	if (!Rcop || !Tco || !Pen || !Feign || Pen->TitleIds.size() < 3 || Rcop->TitleIds.size() < 3 || Pen->Satellites.size() < 3 || !FestHeroEventWorld(Numbered))
	{
		Expect(false, "FestTexts: RCOP 2026, the Championship Online 2027, PENNANT 2027 (three titles, a ladder) and FEIGN 2026 exist");
		return;
	}
	const std::set<std::string> None;
	const auto Id = [](const std::string& Template) { return Template + "@100000"; };

	// The festival branch comes first (§2.9.3): a Championship Online Main win sends T8, and the branch keeps "bracelet-first" back.
	const ss::net::EventTemplate* TcoMain = ss::net::ForEvent(Tco->MainId).FindTemplate(Tco->MainId);
	const sfd::FestResult TcoWin = sfd::FestResultOf(Id(Tco->MainId), 1, false, 123456, None);
	Expect(TcoWin.Branch && TcoWin.Key == "fest-main-tco27" && TcoWin.Body == "The Championship Online Main Event. A bracelet AND the title. Sit down a minute." &&
			   TcoWin.Moment && TcoWin.Moment->Kind == 1 && TcoWin.Moment->Which == Brand::Tco && TcoWin.Moment->Key == "tco27" && TcoWin.Moment->Prize == 123456 &&
			   TcoWin.Moment->Year == 2027,
		"FestTexts: a Championship Online Main win sends the festival line (T8, fest-main-tco27) first, and the champion's moment (Kind 1) waits (" +
			TcoWin.Key + ": \"" + TcoWin.Body + "\"; a bracelet event: " + (TcoMain && TcoMain->Bracelet ? "yes" : "no") + ")");
	// An RCOP Main win: T1 alone (RCOP's Main is a big field: no "You WON it??" beside it).
	const sfd::FestResult RcopWin = sfd::FestResultOf(Id(Rcop->MainId), 1, false, 400000000, None);
	Expect(RcopWin.Branch && RcopWin.Key == "fest-main-rcop" && RcopWin.Body == "You won the RCOP Main?? The Cup goes on the shelf. The shelf goes in the will." &&
			   RcopWin.Body.find("WON it") == std::string::npos && RcopWin.Moment && RcopWin.Moment->Kind == 1,
		"FestTexts: an RCOP Main win sends T1 and no big-field line (\"" + RcopWin.Body + "\")");
	// A championship's Mini and Micro Main: T17 (keyed fest-title-<series id>, the win line's slot) and the title moment (Kind 5).
	const sfd::FestResult Mini = sfd::FestResultOf(Id(Pen->TitleIds[1]), 1, false, 5000, None);
	const sfd::FestResult Micro = sfd::FestResultOf(Id(Pen->TitleIds[2]), 1, false, 500, None);
	const sfd::FestResult RcopMicro = sfd::FestResultOf(Id(Rcop->TitleIds[2]), 1, false, 500, None);
	const std::string PenMini = "The " + ss::fest::NameAfterArticle(Pen->Name) + " Mini Main. A title is a title, kid. Frame something.";
	Expect(Mini.Branch && Mini.Key == "fest-title-" + Pen->SeriesId && Mini.Body == PenMini && Mini.Moment && Mini.Moment->Kind == 5 && Mini.Moment->Mark == 1 &&
			   Micro.Key == Mini.Key && Micro.Body.find(" Micro Main. ") != std::string::npos && Micro.Moment && Micro.Moment->Kind == 5 && Micro.Moment->Mark == 2 &&
			   RcopMicro.Key == "fest-title-rcop" && RcopMicro.Moment && RcopMicro.Moment->Kind == 5,
		"FestTexts: a championship's Mini or Micro Main win sends T17 under fest-title-<series id> and plays the title moment (\"" + Mini.Body + "\" | \"" + Micro.Body + "\")");
	// A Main's final table, 2nd to 9th: T9 ("the {Festival}" drops a leading "The "); 10th sends nothing.
	const sfd::FestResult RcopFt = sfd::FestResultOf(Id(Rcop->MainId), 5, false, 1000, None);
	const sfd::FestResult TcoFt = sfd::FestResultOf(Id(Tco->MainId), 9, false, 1000, None);
	const sfd::FestResult Tenth = sfd::FestResultOf(Id(Rcop->MainId), 10, false, 1000, None);
	Expect(RcopFt.Branch && RcopFt.Key == "fest-ft-rcop" && RcopFt.Body == "Final table of the RCOP 2026 Main Event. Whatever happens next, you were there." && !RcopFt.Moment &&
			   TcoFt.Key == "fest-ft-tco27" && TcoFt.Body.find("the The") == std::string::npos && TcoFt.Body.find("Championship Online 2027 Main Event") != std::string::npos &&
			   !Tenth.Branch && Tenth.Key.empty() && !Tenth.Moment,
		"FestTexts: a Main's final table sends T9 (2nd-9th), 10th nothing (\"" + TcoFt.Body + "\")");
	// Another festival event won: the champion's moment (Kind 3), no festival line (today's lines follow); a satellite: nothing.
	const std::string Sat = Pen->Satellites[0];
	const sfd::FestResult Num = sfd::FestResultOf(Numbered.Id, 1, false, 900, None);
	const sfd::FestResult NumSecond = sfd::FestResultOf(Numbered.Id, 2, false, 600, None);
	const sfd::FestResult SatWin = sfd::FestResultOf(Id(Sat), 1, false, 0, None);
	const sfd::FestResult SatSeat = sfd::FestResultOf(Id(Sat), 1, true, 0, None);
	Expect(!Num.Branch && Num.Moment && Num.Moment->Kind == 3 && Num.Moment->Key == "rcop" && !NumSecond.Moment && !SatWin.Branch && !SatWin.Moment && !SatSeat.Moment,
		"FestTexts: a numbered event won plays Kind 3 with no festival line; a satellite plays nothing (" + Numbered.Id + ")");
	// The cap (§2.9.3): one of win, final table or mark an edition; the belt's is the other slot.
	std::set<std::string> Seen = {"fest-ft-" + Pen->SeriesId};
	const sfd::FestResult Held = sfd::FestResultOf(Id(Pen->TitleIds[1]), 1, false, 5000, Seen);
	const sfd::FestResult HeldFt = sfd::FestResultOf(Id(Rcop->MainId), 3, false, 1000, {"fest-mark-rcop"});
	const sfd::FestResult BeltOnly = sfd::FestResultOf(Id(Rcop->MainId), 1, false, 1000, {"fest-belt-rcop"});
	Expect(Held.Branch && Held.Key.empty() && Held.Body.empty() && Held.Moment && Held.Moment->Kind == 5 && HeldFt.Branch && HeldFt.Key.empty() && BeltOnly.Key == "fest-main-rcop",
		"FestTexts: at most two festival texts an edition: a title after the final-table line and a final table after the mark line send nothing, the belt's never holds the win's back");
	// The seat (§2.9.3, v1.6): T11 only for a seat by the edition's Main ticket; a championship's Step 1 or 2 seat sends the ladder line
	// (FestSeatText ""); RCOP's Main ticket keeps today's line.
	const ss::net::Network& PenNet = ss::net::ForEvent(Pen->MainId);
	const std::string PenTicket = PenNet.TicketOf(Pen->MainId);
	const auto SatId = [](const ss::net::EventTemplate& T) { return T.Id + "@" + std::to_string(T.OnlyDay * 1440 + T.StartMinute); };
	std::string StepLines, MainSeat;
	int Steps = 0;
	for (const std::string& S : Pen->Satellites)
	{
		const ss::net::EventTemplate* T = PenNet.FindTemplate(S);
		if (!T)
		{
			continue;
		}
		const std::string Line = sfd::FestSeatText(SatId(*T), T->SeatTicket, T->BuyInCents);
		if (T->SeatTicket != PenTicket)
		{
			++Steps;
			StepLines += Line;
		}
		else if (MainSeat.empty())
		{
			MainSeat = Line;
			const ss::net::EventTemplate* M = PenNet.FindTemplate(Pen->MainId);
			const std::string Want = "You won a seat in the " + ss::fest::NameAfterArticle(Pen->Name) + " Main Event. " + ss::net::BuyIn(M ? M->BuyInCents : 0) +
				" of tournament for a " + ss::net::BuyIn(T->BuyInCents) + " ticket. Kid.";
			Expect(Line == Want, "FestTexts: a PENNANT seat by its Main ticket sends T11 (\"" + Line + "\")");
		}
	}
	const ss::net::EventTemplate* FeignSat = nullptr;
	for (const std::string& S : Feign->Satellites)
	{
		const ss::net::EventTemplate* T = ss::net::ForEvent(S).FindTemplate(S);
		FeignSat = !FeignSat && T && T->SeatTicket == ss::net::ForEvent(Feign->MainId).TicketOf(Feign->MainId) ? T : FeignSat;
	}
	const std::string FeignSeat = FeignSat ? sfd::FestSeatText(SatId(*FeignSat), FeignSat->SeatTicket, FeignSat->BuyInCents) : std::string();
	// PENNANT's Main ticket is year-free: a 2028 satellite's seat names PENNANT 2028.
	const Festival* Pen28 = FestEditionWorld(Brand::Pennant, 2028);
	const ss::net::EventTemplate* Sat28 = nullptr;
	for (size_t I = 0; Pen28 && I < Pen28->Satellites.size(); ++I)
	{
		const ss::net::EventTemplate* T = PenNet.FindTemplate(Pen28->Satellites[I]);
		Sat28 = !Sat28 && T && T->SeatTicket == PenTicket ? T : Sat28;
	}
	const std::string Seat28 = Sat28 ? sfd::FestSeatText(SatId(*Sat28), Sat28->SeatTicket, Sat28->BuyInCents) : std::string();
	Expect(Steps == 4 && StepLines.empty() && !MainSeat.empty() && FeignSeat.rfind("You won a seat in the FEIGN 2026 Main Event. ", 0) == 0 &&
			   (!Pen28 || Seat28.rfind("You won a seat in the " + ss::fest::NameAfterArticle(Pen28->Name) + " Main Event. ", 0) == 0) &&
			   sfd::FestSeatText(Id(Rcop->Satellites[0]), "rcop-main", 220).empty(),
		"FestTexts: T11 only for a Main-ticket seat; PENNANT's Step 1 and Step 2 seats (two days each) send the ladder line, RCOP's Main seat today's (" +
			std::to_string(Steps) + " step seats; \"" + StepLines + "\"; \"" + FeignSeat + "\"; \"" + Seat28 + "\")");
	// T12: gh0stfold's line filled from step1's buy-in, byte for byte today's 83 characters.
	const std::string T12 = ss::site::FillCopy(ss::fest::fest_text_detail::FestText("T12"), {{"Step1", sfd::FestStep1Fill()}});
	Expect(T12 == "gg on the title. enjoy it. the RCOP satellites start at $2.20, see you on the steps" && T12.size() == 83,
		"FestTexts: gh0stfold's filled line equals today's 83 characters (\"" + T12 + "\", " + std::to_string(T12.size()) + ")");

	// The session (FestStep's keyed lines): a skip over the marks sends the belt's T10 and PODIUM's T14 once; the Championship Online's
	// FINALIST is held (its final-table line went); a second skip, a step and a reload send nothing again.
	const FestSessionRunWorld& Run = FestSessionWorld();
	const std::string Want1 = "There's a belt. You have to wear the belt.|PODIUM at Ring Rush 2027. 2 of 500 in the race. I'm telling people.";
	Expect(Run.Skip1 == Want1, "FestTexts: a WorldSkip steps the marks: the belt's T10 and PODIUM's T14, each once, the capped FINALIST none (\"" + Run.Skip1 + "\")");
	Expect(Run.Parsed && Run.Again.empty() && Run.Reloaded.empty() && Run.MarksBack == 3,
		"FestTexts: the belt and mark texts are keyed: a second skip, a step and a reload (" + std::to_string(Run.MarksBack) + " marks back) send none (\"" + Run.Again +
			"\" | \"" + Run.Reloaded + "\")");
	std::printf("  FestTexts: the session half in %.0f ms\n", Run.Ms);
}

void FestMoments()
{
	// §2.8.5: the belt's moment first, then each mark's in crown order; DoneFestMoment keys the one shown (festbelt:, festmark: in the
	// sites' book, kept by the save); a reload before the dismiss plays it again.
	const FestSessionRunWorld& Run = FestSessionWorld();
	Expect(Run.Peek1 == "2 rcop 1700 m5 1/608" && Run.Peek2 == "4 ring27 900 m4 2/500",
		"FestMoments: the belt's moment (Kind 2, the race's points) plays first, then PODIUM's (Kind 4) (" + Run.Peek1 + " | " + Run.Peek2 + ")");
	Expect(Run.Peek3 == "4 ring27 900 m4 2/500" && Run.Peek4 == "4 tco27 600 m3 7/1234" && Run.Peek5 == "-",
		"FestMoments: after a reload the belt stays seen and the undismissed mark plays again, then the next, then none (" + Run.Peek3 + " | " + Run.Peek4 + " | " + Run.Peek5 + ")");
	Expect(Run.BeltKeyKept && Run.MarkKeyKept, "FestMoments: the dismissed moments' done keys are in the save (festbelt:rcop, festmark:ring27, festmark:tco27)");
}

void FestRerunStories()
{
	// §2.9.4: Rerun's three festival prefixes, matched with rfind(prefix, 0) == 0; the facts packed when the story is noted fill V1-V9.
	namespace sfd = ss::session_fest_detail;
	namespace video = ss::video;
	const ss::fest::Festival* Pen = FestEditionWorld(ss::fest::Brand::Pennant, 2027);
	if (!Pen)
	{
		Expect(false, "FestRerunStories: PENNANT 2027 exists");
		return;
	}
	const std::string Key = "fest-main-" + Pen->SeriesId;
	Expect(sfd::FestStoryPrefix(Key) == 0 && sfd::FestStoryPrefix("fest-belt-rcop") == 1 && sfd::FestStoryPrefix("fest-ft-tco27") == 2 &&
			   sfd::FestStoryPrefix("fest-title-rcop") < 0 && sfd::FestStoryPrefix("fest-mark-rcop") < 0 && sfd::FestStoryPrefix("mm-title") < 0,
		"FestRerunStories: fest-main-, fest-belt- and fest-ft- keys are stories; a title, a mark or any other key is not");
	const ss::world::World Fresh = FestFreshWorld();
	std::string Facts;
	for (const std::pair<std::string, std::string>& F : sfd::FestStoryFacts(Fresh, Key))
	{
		Facts += (Facts.empty() ? "" : ";") + F.first + "=" + video::vcontent::FactEsc(F.second);
	}
	video::Candidate C;
	C.Id = "story:" + Key;
	C.K = video::Kind::Story;
	C.Event = Key;
	C.Source = 4;
	C.Series = true;
	C.IdeaFacts = Facts;
	std::string Got;
	bool Each = true;
	const char* const Want[] = {"I won the PENNANT 2027 Main Event", "THE GOLD PENNANT IS MINE", "How I won PENNANT 2027 from a "};
	for (int S = 0; S < 3; ++S)
	{
		video::Choice Ch;
		Ch.Candidates = {C.Id};
		Ch.T = static_cast<video::TitleStyle>(S);
		const std::vector<video::TitleOption> Opts = video::TitleOptions(Ch, {C}, video::Channel(), 0, 3);
		const std::string First = Opts.empty() ? std::string() : Opts.front().Text;
		const std::vector<std::string> U2 = video::video_copy::FestStoryLines(Key, Ch.T, Facts);
		Each = Each && !Opts.empty() && First.rfind(Want[S], 0) == 0 && Opts.front().Key.rfind("story.fest-main.", 0) == 0 && U2.size() == 1 && U2.front().rfind(Want[S], 0) == 0;
		Got += (S ? " | " : "") + First;
	}
	Expect(Each, "FestRerunStories: a PENNANT 2027 Main win's story leads with V1-V3 in their styles, filled from its facts (" + Got + "; facts " + Facts + ")");
	// §2.11's articles rule on a name that starts with "The ": V1 and V4 (after "the ") drop it, V3 and V6 print it whole, and no
	// title in Rerun 2.0's options or Update 2's lines holds "the The" (any case).
	const ss::fest::Festival* Tco = FestEditionWorld(ss::fest::Brand::Tco, 2027);
	Expect(Tco && Tco->Name.rfind("The ", 0) == 0, "FestRerunStories: The Championship Online 2027 exists, its name starting with \"The \"");
	auto TheTheTco = [](const std::string& S) {
		std::string L = S;
		for (char& Ch : L)
		{
			Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
		}
		return L.find("the the ") != std::string::npos;
	};
	std::string TcoGot;
	bool TcoEach = Tco != nullptr && Tco->Name.size() > 4;
	for (int Belt = 0; TcoEach && Belt < 2; ++Belt)
	{
		const std::string TcoKey = (Belt ? "fest-belt-" : "fest-main-") + Tco->SeriesId;
		std::string TcoFacts;
		for (const std::pair<std::string, std::string>& F : sfd::FestStoryFacts(Fresh, TcoKey))
		{
			TcoFacts += (TcoFacts.empty() ? "" : ";") + F.first + "=" + video::vcontent::FactEsc(F.second);
		}
		const std::string Bare = Tco->Name.substr(4); // "Championship Online 2027"
		const std::string WantTco[2][3] = {{"I won the " + Bare + " Main Event", "", "How I won " + Tco->Name + " from a "},
			{"I won the " + Bare + " belt", "", "How I became " + Tco->Name + "'s Player of the Series"}};
		video::Candidate TC = C;
		TC.Id = "story:" + TcoKey;
		TC.Event = TcoKey;
		TC.IdeaFacts = TcoFacts;
		for (int S = 0; S < 3; ++S)
		{
			video::Choice Ch;
			Ch.Candidates = {TC.Id};
			Ch.T = static_cast<video::TitleStyle>(S);
			const std::vector<video::TitleOption> Opts = video::TitleOptions(Ch, {TC}, video::Channel(), 0, 3);
			const std::vector<std::string> U2 = video::video_copy::FestStoryLines(TcoKey, Ch.T, TcoFacts);
			const std::string First = Opts.empty() ? std::string() : Opts.front().Text;
			const std::string& TcoWant = WantTco[Belt][S];
			TcoEach = TcoEach && !Opts.empty() && Opts.front().Key.rfind(Belt ? "story.fest-belt." : "story.fest-main.", 0) == 0 && U2.size() == 1 &&
				(TcoWant.empty() || (First.rfind(TcoWant, 0) == 0 && U2.front().rfind(TcoWant, 0) == 0));
			for (const video::TitleOption& O : Opts)
			{
				TcoEach = TcoEach && !TheTheTco(O.Text);
			}
			for (const std::string& Line : U2)
			{
				TcoEach = TcoEach && !TheTheTco(Line);
			}
			TcoGot += (TcoGot.empty() ? "" : " | ") + First + (U2.empty() ? std::string(" / -") : " / " + U2.front());
		}
	}
	Expect(TcoEach, "FestRerunStories: The Championship Online 2027's Main win and belt stories print \"the Championship Online 2027\" in V1 "
					"and V4 and keep \"The\" in V3 and V6, in both fills, never \"the The\" (" + TcoGot + ")");
	std::printf("  FestRerunStories: TCO 2027 %s\n", TcoGot.c_str());
	// The session: the belt's text becomes Rerun's story at the next calendar step, a series event's (x1.3) with its facts packed; a
	// festival line sent before the channel had state is marked told; a reload reads the facts again (a story's aren't saved).
	const FestSessionRunWorld& Run = FestSessionWorld();
	Expect(Run.Story == "Festival=" + video::vcontent::FactEsc("RCOP 2026") + ";N=0 series" && Run.OldStory == "-",
		"FestRerunStories: the belt's text is noted as a story (series, its facts packed) and the old final table's is not (" + Run.Story + " | " + Run.OldStory + ")");
	Expect(Run.StoryLoaded != "-" && Run.StoryBack == Run.Story,
		"FestRerunStories: after a reload the story's facts are read again at the next step (" + Run.StoryLoaded + " -> " + Run.StoryBack + ")");
	std::printf("  FestRerunStories: noted \"%s\", reloaded \"%s\", stepped \"%s\"\n", Run.Story.c_str(), Run.StoryLoaded.c_str(), Run.StoryBack.c_str());
}

// ------------------------------------------------------------------ FW-6: FestStep's tickets, refunds and letter; ss.Fest.*

/** One mark planted with its crown ticket unpaid (world\tfest\tmark's trailing t, §2.13). */
void FestPlantTicketWorld(ss::world::World& W, const std::string& Key, ss::world::FestMark Mark, int Rank, int Scorers, int Points)
{
	W.Read({"world", "fest", "mark", Key, std::to_string(static_cast<int>(Mark)), std::to_string(Rank), std::to_string(Scorers), std::to_string(Points), "t"});
}

/** A session's wallet as one line: its tickets ("<id>x<n>") and the Guest List's holds ("hold:<key>"). */
std::string FestWalletWorld(const ss::Session& S)
{
	std::string Out;
	for (const std::pair<const std::string, int>& T : S.Life.Tickets)
	{
		Out += T.second > 0 ? (Out.empty() ? "" : " ") + T.first + "x" + std::to_string(T.second) : std::string();
	}
	for (const ss::site::play::Hold& H : S.Life.SitePlay.Holds)
	{
		Out += (Out.empty() ? "" : " ") + std::string("hold:") + H.Key;
	}
	return Out;
}

/** The crown letters in a session's Mail ("<thread>:<subject>", "|"-joined, oldest first). */
std::string FestCrownLettersWorld(const ss::Session& S)
{
	std::string Out;
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		Out += M.Subject.find(": the belt is yours") != std::string::npos ? (Out.empty() ? "" : "|") + M.Thread + ":" + M.Subject : std::string();
	}
	return Out;
}

/** The marks in a world still holding an unpaid crown ticket. */
int FestUnpaidWorld(const ss::world::World& W)
{
	int N = 0;
	for (const ss::world::HeroFestMark& K : ss::world::HeroMarks(W))
	{
		N += K.Ticket ? 1 : 0;
	}
	return N;
}

/** The session half of FestCrownTickets and FestMainTicket's world half, run once: a career skipped into RCOP 2026's window (day 19);
 *  ss.Fest.Result pennant cash before PENNANT exists; eight marks planted through the world text (six with their t: RCOP 2026's and
 *  PENNANT 2027's belts, Ring Rush 2027's PODIUM, the Championship Online 2027's FINALIST at rank 4 of 600, WRIT 2027's FINALIST,
 *  Summer Slam 2027's PODIUM; GRIT 2027's TOP 10% at rank 26 of 600 and PUSH 2027's FINALIST without its t, as a crown with
 *  kCrownTickets false writes it); a step, a second step and a skip; ss.Fest.Describe; a reload from the save's text and a step;
 *  ss.Fest.Result rcop cash; ss.Fest.Moment rcop mark podium, dismissed. */
struct FestTicketRunWorld
{
	std::string NotYet, Describe, Result, MomentAnswer, Peek, PeekAfter;
	std::string Wallet1, Wallet2, WalletBack;    // the tickets and holds after the first step, after more, after the reload
	std::string Letters1, Letters2, LettersBack; // the crown letters
	std::string LetterBody;                      // RCOP 2026's crown letter
	std::string Keys;                            // the festticket: keys in the last save
	int Unpaid1 = -1, Unpaid2 = -1, UnpaidBack = -1;
	bool Parsed = false, NoDevKey = false;
	int ResultPlace = 0, ResultPaid = -1, Rank = 0;
	double Ms = 0.0;
};
FestTicketRunWorld& FestTicketsWorld()
{
	static std::unique_ptr<FestTicketRunWorld> Run;
	if (Run)
	{
		return *Run;
	}
	Run = std::make_unique<FestTicketRunWorld>();
	namespace world = ss::world;
	const auto T0 = std::chrono::steady_clock::now();
	FestHooksWorld H0;
	ss::SaveData D;
	{
		ss::Session S0(H0, "festtickets");
		S0.WorldSkip(18); // into RCOP 2026's window (days 17-34)
		D = H0.Last;
	}
	FestHooksWorld H1;
	{
		ss::Session S(H1, "festtickets", &D);
		Run->NotYet = S.DebugFest("result pennant cash");
		world::World& W = S.Living();
		FestPlantTicketWorld(W, "rcop", world::FestMark::Title, 1, 608, 1700);
		FestPlantTicketWorld(W, "ignite.pennant27", world::FestMark::Title, 1, 700, 2100);
		FestPlantTicketWorld(W, "ring27", world::FestMark::Podium, 2, 500, 900);
		FestPlantTicketWorld(W, "tco27", world::FestMark::Finalist, 4, 600, 650);
		FestPlantTicketWorld(W, "runittwice.writ27", world::FestMark::Finalist, 3, 150, 300);
		FestPlantTicketWorld(W, "slam27", world::FestMark::Podium, 3, 400, 500);
		FestPlantMarkWorld(W, "runittwice.grit27", world::FestMark::Top10, 26, 600, 200);
		FestPlantMarkWorld(W, "ignite.push27", world::FestMark::Finalist, 5, 300, 400);
		S.FestStep(S.WorldMinutes(), S.WorldMinutes() + 60.0);
		Run->Wallet1 = FestWalletWorld(S);
		Run->Letters1 = FestCrownLettersWorld(S);
		Run->Unpaid1 = FestUnpaidWorld(S.Living());
		S.FestStep(S.WorldMinutes(), S.WorldMinutes() + 60.0);
		S.WorldSkip(1);
		Run->Wallet2 = FestWalletWorld(S);
		Run->Letters2 = FestCrownLettersWorld(S);
		Run->Unpaid2 = FestUnpaidWorld(S.Living());
		Run->Describe = S.DebugFest("describe");
		for (const ss::life::Letter& M : S.Life.Inbox)
		{
			Run->LetterBody = M.Subject == "RCOP 2026: the belt is yours" ? M.Body : Run->LetterBody;
		}
	}
	FestHooksWorld H2;
	ss::SaveData Back;
	Run->Parsed = ss::SaveData::Parse(H1.Last.Serialize(), Back);
	{
		ss::Session S2(H2, "festtickets", &Back);
		S2.FestStep(S2.WorldMinutes(), S2.WorldMinutes() + 60.0);
		Run->WalletBack = FestWalletWorld(S2);
		Run->LettersBack = FestCrownLettersWorld(S2);
		Run->UnpaidBack = FestUnpaidWorld(S2.Living());
		Run->Result = S2.DebugFest("result rcop cash");
		const std::vector<world::HeroFestResult> Rows = world::HeroFestResults(S2.Living(), "rcop");
		if (!Rows.empty())
		{
			ss::net::EventInstance E;
			Run->ResultPlace = Rows.front().Place; // newest first
			Run->ResultPaid = ss::net::Shared().FindInstance(Rows.front().EventId, E) ? static_cast<int>(ss::net::Shared().Payouts(E).size()) : -1;
		}
		Run->Rank = world::FestStanding(S2.Living(), -1).first;
		Run->MomentAnswer = S2.DebugFest("moment rcop mark podium");
		Run->Peek = FestMomentLineWorld(S2.PeekFestMoment());
		S2.DoneFestMoment();
		Run->PeekAfter = FestMomentLineWorld(S2.PeekFestMoment());
		Run->NoDevKey = S2.Life.Sites.Done.count("festmark:rcop") == 0;
		for (const std::string& K : H2.Last.Life.Sites.Done)
		{
			Run->Keys += K.rfind("festticket:", 0) == 0 ? (Run->Keys.empty() ? "" : " ") + K : std::string();
		}
	}
	Run->Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	return *Run;
}

/**
 * FestCrownTickets (§2.15.1, §2.6.1 step 7, §2.13): FestStep pays each crown ticket once, keyed festticket:<key> in the sites' book, and
 * drops the mark's t: THE BELT the next edition's Main ticket (RCOP's rcop-main into the wallet though its Main is locked; PENNANT's
 * year-free ignite.pennant-main through the Value Governor, over the cap on the Guest List), PODIUM its Mega (ring28-mega), FINALIST its
 * Last Chance (rank 4 of 600: tco28-last; WRIT's The Summons), Summer Slam's PODIUM nothing (no satellite) but its key; rank 26 (TOP
 * 10%) and a FINALIST without its t (kCrownTickets false) none. Nothing again after a second step, a skip, or a save-and-reload; the
 * crown letter once for each belt, from the host (L5-L6). ss.Fest.Describe and ss.Fest.Result rcop cash answer, and the race reacts;
 * ss.Fest.Moment rcop mark podium plays a DEV PODIUM moment that keeps no key; "not yet" before the brand's festival exists.
 * Planted bugs: a ticket for every mark (the grade and the t ignored); the key not written; FestTicketPaid not called; the PODIUM and
 * FINALIST seats swapped; a rival's ticket straight into the wallet (no Governor); the crown letter missing; DebugFest's result not
 * told to the world; Describe without the marks; the mark verb's grade misread.
 */
void FestCrownTickets()
{
	const FestTicketRunWorld& Run = FestTicketsWorld();
	const std::string Want = "rcop-mainx1 ring28-megax1 tco28-lastx1";
	const bool Writ = (Run.Wallet1.find("runittwice.writ28-sat1x1") != std::string::npos) !=
		(Run.Wallet1.find(":t:runittwice.writ28-sat1:festticket:runittwice.writ27") != std::string::npos);
	const bool Pennant = Run.Wallet1.find(":t:ignite.pennant-main:festticket:ignite.pennant27") != std::string::npos &&
		Run.Wallet1.find("ignite.pennant-mainx") == std::string::npos;
	const std::string& Bare = Run.Wallet1;
	std::string Plain; // the wallet without its WRIT and PENNANT entries
	size_t From = 0;
	while (From <= Bare.size())
	{
		size_t End = Bare.find(' ', From);
		End = End == std::string::npos ? Bare.size() : End;
		const std::string Item = Bare.substr(From, End - From);
		if (!Item.empty() && Item.find("writ28") == std::string::npos && Item.find("pennant") == std::string::npos)
		{
			Plain += (Plain.empty() ? "" : " ") + Item;
		}
		From = End + 1;
	}
	Expect(Plain == Want && Writ && Pennant && Run.Unpaid1 == 0,
		"FestCrownTickets: each mark's ticket once: THE BELT the next Main's (rcop-main into the wallet, locked Main or not; PENNANT's year-free "
		"ignite.pennant-main held on the Guest List over the cap), PODIUM the Mega (ring28-mega), FINALIST the Last Chance (rank 4 of 600: "
		"tco28-last; WRIT's The Summons), Summer Slam's none; rank 26 of 600 and a FINALIST without its t none; every t dropped (" + Run.Wallet1 +
		"; unpaid " + std::to_string(Run.Unpaid1) + ")");
	Expect(Run.Wallet2 == Run.Wallet1 && Run.Parsed && Run.WalletBack == Run.Wallet1 && Run.Unpaid2 == 0 && Run.UnpaidBack == 0,
		"FestCrownTickets: a second step, a skip and a save-and-reload pay nothing again (after: '" + Run.Wallet2 + "'; reloaded: '" + Run.WalletBack + "')");
	Expect(Run.Keys == "festticket:ignite.pennant27 festticket:rcop festticket:ring27 festticket:runittwice.writ27 festticket:slam27 festticket:tco27",
		"FestCrownTickets: the paid tickets' keys are in the save's site\\tdone line, Summer Slam's too, GRIT's and PUSH's not (" + Run.Keys + ")");
	const std::string Letters = "riverline:RCOP 2026: the belt is yours|ignite:PENNANT 2027: the belt is yours";
	Expect(Run.Letters1 == Letters && Run.Letters2 == Letters && Run.LettersBack == Letters &&
			   Run.LetterBody == "Congratulations. You finished first in the RCOP 2026 race with 1,700 points, and the belt is yours. Your seat in the next RCOP Main Event "
								 "comes with it. See you at the next one.",
		"FestCrownTickets: the crown letter once for each belt, from its host, through a second step, a skip and a reload (" + Run.Letters1 + " | " + Run.LettersBack +
			"; '" + Run.LetterBody + "')");
	const bool Described = Run.Describe.find("Your marks: RCOP 2026 THE BELT, 1 of 608, 1,700 pts, crown ticket paid") != std::string::npos &&
		Run.Describe.find("Race: RCOP 2026") != std::string::npos && Run.Describe.find("Ignite's Pass: rents ") != std::string::npos &&
		Run.Describe.find("RunItTwice's Pass: rents ") != std::string::npos && Run.Describe.find("2026's windows:\n  RCOP 2026 (RiverLine): Oct 18 - ") != std::string::npos;
	Expect(Described && Run.NotYet.rfind("not yet: ", 0) == 0,
		"FestCrownTickets: ss.Fest.Describe lists this year's windows, the live race, the marks with their tickets and each rival's Pass; a brand's verb says "
		"'not yet' before its festival exists ('" + Run.NotYet + "')");
	std::printf("  [FestCrownTickets] describe:\n%s\n", Run.Describe.c_str());
	Expect(Run.Result.rfind("result rcop cash: ", 0) == 0 && Run.Result.find("(dev)") != std::string::npos && Run.ResultPlace > 0 && Run.ResultPlace == Run.ResultPaid &&
			   Run.Rank > 0,
		"FestCrownTickets: ss.Fest.Result rcop cash plants a min cash in RCOP 2026's next numbered event through the world's result path, logged as dev, and the "
		"race ranks the hero ('" + Run.Result + "'; place " + std::to_string(Run.ResultPlace) + " of " + std::to_string(Run.ResultPaid) + " paid, rank " +
			std::to_string(Run.Rank) + ")");
	Expect(Run.MomentAnswer.find("(dev)") != std::string::npos && Run.Peek == "4 rcop 600 m4 2/500 dev" && Run.PeekAfter != Run.Peek && Run.NoDevKey,
		"FestCrownTickets: ss.Fest.Moment rcop mark podium plays a DEV PODIUM moment (Kind 4) that grants nothing and keeps no key (" + Run.Peek + " -> " + Run.PeekAfter + ")");
	std::printf("  [FestCrownTickets] session run %.0f ms\n", Run.Ms);
}

/** A life with an Ignite account (the step refunds' wallet). */
ss::life::State FestIgniteLifeWorld()
{
	ss::life::State L;
	ss::site::Account A;
	A.Site = ss::net::SiteId::Ignite;
	L.Sites.Accounts.push_back(A);
	return L;
}

/**
 * FestStepCredit (§2.15.1, §2.6.1 item 8, v1.6): after PENNANT 2027's crown, a hero holding an unused Step 3 ticket and two Step 2
 * tickets gets their SeatValue in Ignite's wallet once each (keyed festcredit:<ticket>, one ledger line each, "Unused PENNANT 2027 Step
 * {K} ticket refunded"), however many hours the credit runs; nothing before the crown; a reload after the crown pays nothing more, the
 * ticket handed back or not; the held Main ticket (and PENNANT 2028's step ticket) untouched; after PENNANT 2040's crown, with no Main
 * left, the Main ticket still untouched.
 * Planted bugs: the credit run every hour (the key unchecked and the ticket kept: paid twice); the Main ticket credited (FestStepOf's
 * Main exclusion dropped); the credit before the crown.
 */
void FestStepCredit()
{
	namespace sfd = ss::session_fest_detail;
	namespace world = ss::world;
	using ss::fest::Brand;
	const ss::fest::Festival* P27 = FestEditionWorld(Brand::Pennant, 2027);
	const ss::fest::Festival* P28 = FestEditionWorld(Brand::Pennant, 2028);
	const ss::fest::Festival* P40 = FestEditionWorld(Brand::Pennant, 2040);
	if (!P27 || !P28 || !P40)
	{
		Expect(false, "FestStepCredit: PENNANT 2027, 2028 and 2040 exist");
		return;
	}
	const ss::net::Network& Ig = ss::net::Site(ss::net::SiteId::Ignite);
	const ss::net::EventTemplate* S1 = Ig.FindTemplate(P27->SeriesId + "-s1a");
	const ss::net::EventTemplate* S2 = Ig.FindTemplate(P27->SeriesId + "-s2a");
	const ss::net::EventTemplate* S2Next = Ig.FindTemplate(P28->SeriesId + "-s2a");
	const std::string Main = Ig.TicketOf(P27->MainId);
	const std::string Step2 = S1 ? S1->SeatTicket : std::string(), Step3 = S2 ? S2->SeatTicket : std::string();
	const std::string Step3Next = S2Next ? S2Next->SeatTicket : std::string();
	const double Crown27 = ss::fest::LastEnd(*P27) + 60.0, Crown40 = ss::fest::LastEnd(*P40) + 60.0;
	const world::World Fresh = FestFreshWorld();
	const world::Npc* Champ = Fresh.Get(3);
	const std::string Name = Champ ? Champ->Name : std::string("x");
	world::World W;
	FestLoadWorld(W, FestPlantFirstWorld(FestTextWorld(Fresh), FestHonorLineWorld(2027, P27->Name + " Player of the Series", P27->SeriesId + "#pos", 3, Name, 0, 2100, Crown27) +
															  FestHonorLineWorld(2040, P40->Name + " Player of the Series", P40->SeriesId + "#pos", 3, Name, 0, 2100, Crown40)));
	ss::life::State L = FestIgniteLifeWorld();
	L.Tickets[Step3] = 1;
	L.Tickets[Step2] = 2;
	L.Tickets[Main] = 1;
	L.Tickets[Step3Next] = 1;
	const int Before = sfd::FestStepCredits(L, W, P27->MainStart + 30.0); // the steps are over, the crown not yet
	int Credited = 0;
	for (int Hour = 0; Hour < 24; ++Hour)
	{
		Credited += sfd::FestStepCredits(L, W, Crown27 + 60.0 * Hour);
	}
	const ss::site::Account* A = ss::site::AccountOf(L.Sites, ss::net::SiteId::Ignite);
	const ss::Chips Want = Ig.SeatValue(Step3) + 2 * Ig.SeatValue(Step2);
	std::string Lines;
	for (const ss::site::Line& Ln : A ? A->Lines : std::vector<ss::site::Line>())
	{
		Lines += (Lines.empty() ? "" : "|") + Ln.Label + " " + ss::Money(Ln.Cents);
	}
	const std::string WantLines = "Unused PENNANT 2027 Step 2 ticket refunded " + ss::Money(2 * Ig.SeatValue(Step2)) + "|Unused PENNANT 2027 Step 3 ticket refunded " +
		ss::Money(Ig.SeatValue(Step3));
	const bool Kept = L.Tickets.count(Step3) == 0 && L.Tickets.count(Step2) == 0 && L.Tickets[Main] == 1 && L.Tickets[Step3Next] == 1;
	Expect(!Step3.empty() && !Step2.empty() && Step3 != Main && Before == 0 && Credited == 2 && A && A->Cents == Want && Lines == WantLines && Kept &&
			   L.Sites.Done.count("festcredit:" + Step3) > 0 && L.Sites.Done.count("festcredit:" + Step2) > 0,
		"FestStepCredit: after PENNANT 2027's crown the unused Step 3 ticket (" + Step3 + ") and two Step 2 tickets are credited at their SeatValue once, "
		"however many hours the credit runs, one ledger line each, keyed festcredit:<ticket>; nothing before the crown (" + std::to_string(Before) +
			"); the Main ticket and PENNANT 2028's step ticket untouched (credited " + std::to_string(Credited) + ", wallet " + ss::Money(A ? A->Cents : 0) + " of " +
			ss::Money(Want) + "; '" + Lines + "')");
	// A reload after the crown: the save's text read back, the ticket handed back as an old save might hold it.
	ss::SaveData D;
	D.Life = L;
	ss::SaveData Back;
	const bool Parsed = ss::SaveData::Parse(D.Serialize(), Back);
	Back.Life.Tickets[Step3] = 1;
	const int Again = sfd::FestStepCredits(Back.Life, W, Crown27 + 3.0 * 1440.0);
	const ss::site::Account* B = ss::site::AccountOf(Back.Life.Sites, ss::net::SiteId::Ignite);
	Expect(Parsed && Again == 0 && B && B->Cents == Want && Back.Life.Tickets[Step3] == 1,
		"FestStepCredit: a reload after the crown pays nothing more, even with the ticket back in the wallet (credited " + std::to_string(Again) + ")");
	// PENNANT 2040: no Main is left to place the year-free Main ticket, and still it is never credited.
	ss::life::State L40 = FestIgniteLifeWorld();
	L40.Tickets[Main] = 1;
	ss::net::EventInstance None;
	const bool NoMain = !Ig.NextFor(Main, P40->MainStart + 1.0, None);
	const int Last = sfd::FestStepCredits(L40, W, Crown40 + 60.0);
	Expect(NoMain && Last == 0 && L40.Tickets[Main] == 1,
		"FestStepCredit: after PENNANT 2040's crown, with no Main left to take it, the held Main ticket is still untouched (credited " + std::to_string(Last) + ")");
}

/**
 * FestMainTicket, F-world's half (§2.15.1, v1.6): THE BELT of a championship edition earns the year-free Main ticket, so a seat covered
 * after the next edition's Main has run registers on the one after: PENNANT 2027's belt ticket, asked after PENNANT 2028's Main started,
 * enters PENNANT 2029's Main (Network::NextFor, and the lobby's ticket count through Life::TicketsFor); REIGN's the same.
 * Planted bug: the Main keyed by its edition's id (the late seat is dead).
 */
void FestMainTicketWorld()
{
	namespace sfd = ss::session_fest_detail;
	using ss::fest::Brand;
	std::string Bad;
	for (const Brand B : {Brand::Pennant, Brand::Reign})
	{
		const ss::fest::Festival* F27 = FestEditionWorld(B, 2027);
		const ss::fest::Festival* F28 = FestEditionWorld(B, 2028);
		const ss::fest::Festival* F29 = FestEditionWorld(B, 2029);
		if (!F27 || !F28 || !F29)
		{
			Bad += " no 2027-2029 editions";
			continue;
		}
		const std::string Ticket = sfd::FestCrownTicket(*F27, static_cast<int>(ss::world::FestMark::Title));
		const ss::net::Network& N = ss::net::ForEvent(F27->MainId);
		ss::net::EventInstance E;
		const bool Placed = N.NextFor(Ticket, F28->MainStart + 1.0, E);
		const std::string Template = Placed ? E.Id.substr(0, E.Id.find('@')) : std::string();
		ss::life::State L;
		L.Tickets[Ticket] = 1;
		const bool Registers = Placed && Template == F29->MainId && L.TicketsFor(N.TicketOf(Template)) == 1;
		if (!Registers)
		{
			Bad += " " + std::string(ss::fest::BrandKey(B)) + ": '" + Ticket + "' -> '" + Template + "'";
		}
	}
	Expect(Bad.empty(), "FestMainTicket: a championship belt's year-free Main ticket, covered after the next Main ran, registers on the one after (PENNANT and REIGN 2027's "
						"belts on the 2029 Mains)" + Bad);
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
	// FW-3: the roll, the defender, the titles, Discover, the pins and the headlines
	if (Only("FestRollFromHonors"))
	{
		FestRollFromHonors();
	}
	if (Only("FestBlankHonesty"))
	{
		FestBlankHonesty();
	}
	if (Only("FestDefending"))
	{
		FestDefending();
	}
	if (Only("FestDiscoverWidened"))
	{
		FestDiscoverWidened();
	}
	if (Only("FestHeroPinsDerived"))
	{
		FestHeroPinsDerived();
	}
	if (Only("FestHeadline"))
	{
		FestHeadline();
	}
	if (Only("FestTitleOrder"))
	{
		FestTitleOrder();
	}
	if (Only("FestChampTitles"))
	{
		FestChampTitlesWorld(); // F-world's world half (F-net's in festivals_main.cpp)
	}
	if (Only("FestMajorFloor"))
	{
		FestMajorFloorWorld(); // F-world's world half (F-net's in festivals_main.cpp)
	}
	// FW-4: WHO'S IN
	if (Only("FestFieldStories"))
	{
		FestFieldStories();
	}
	if (Only("FestStoryHooks"))
	{
		FestStoryHooks();
	}
	// FW-5: the session's texts, moments and Rerun
	if (Only("FestTexts"))
	{
		FestTexts();
	}
	if (Only("FestMoments"))
	{
		FestMoments();
	}
	if (Only("FestRerunStories"))
	{
		FestRerunStories();
	}
	// FW-6: FestStep's tickets, refunds and letter; ss.Fest.*
	if (Only("FestCrownTickets"))
	{
		FestCrownTickets();
	}
	if (Only("FestStepCredit"))
	{
		FestStepCredit();
	}
	if (Only("FestMainTicket"))
	{
		FestMainTicketWorld(); // F-world's world half (F-net's in festivals_main.cpp)
	}
}
} // namespace festivals_test
