// Update 4, Festival Season (SPEC v1.7 §2.5-§2.6): the world's festival race, its crown, the hero's marks and the views the hubs,
// the card and the roll read (H/WorldFest.h), and WHO'S IN (World::FieldStoriesOf, §4.7.3). F-world's file from G1 (S-promote adds
// the crowd's race scores at G4).
//
// Phase A (§6 PA-19, PA-23-PA-25, PA-27; the lead): stubs that keep today's behaviour. IsCrownHonor is the spec's predicate (no
// Honor's id ends in "#pos" before G1, so its two skips change nothing).
//
// G1, FW-2a (F-world; PLAN_FWORLD §4 FW-2, Corrections item 1): the race and its marks (§2.5.1-§2.5.4). FestDay opens a festival's
// race on its FirstDay (its site open in this world, no crown Honor for its key); FestScoreResult and FestScoreHero add a cash's
// fest::RacePoints to the scorer's row (online, the live key's series, never a satellite, Place >= 1, points > 0; the hero's result once,
// World::HeroFinished's !Again); FestRank orders a race (points, then a person ahead of the hero, then the lower id); MarkFor,
// FestLastRank and FestPointsTo (G/WorldFestDetail.h, R-4) and fest::MarkCutoff are the marks' rule and the ladder; FestTop,
// FestStanding, FestKnown and FestRaceKey read the live race, else the last crown's top ten (FestLast, the Afterglow). The ranked
// race is kept in World's FestViews (R-10; a copied world starts without it), rebuilt when the world's stamp (serial, Rev, HeroRev)
// moves or a race writer here drops it. No RNG (§2.12): sums and a sort with a total order.
//
// G1, FW-2b (F-world; PLAN_FWORLD Corrections item 1): the crown (§2.6.1 steps 1-7). FestHour, from World::AdvanceTo's hourly block,
// crowns the live race in the first hour slice at or after fest::LastEnd with no Pending of its series left in the queue (the hero
// still seated in its Main holds it, up to the two days the world waits for them), so every skip crowns in the same slice; it clears
// FestLast once the Afterglow's three days have run from the crown. The crown ranks the race (S scorers; none: the race just closes,
// no champion), pushes the Player of the Series Honor (EventId "<series>#pos", Entries the points), posts EventKind::FestivalCrown
// with the site's bits (and FlagHero for the hero's), gives a person who wins +6 online fame, keeps the hero's mark (with its crown
// ticket from FINALIST up while fest::kCrownTickets), keeps the top ten in FestLast and closes the race. FestDay crowns a stale race
// first (the calendar never leaves one). A crowned key never reopens and never crowns twice. FestCrownedAt reads the crown Honors
// (kept per world state beside the race view); HeroMark and HeroMarks read the hero's marks; FestTicketPaid (G/WorldFestDetail.h)
// is the session's receipt for a paid crown ticket. Step 8's refunds and the tickets' payment are the session's (FestStep, FW-6).
//
// G1, FW-3 (F-world; SPEC v1.7 §2.6.2-§2.6.5, §2.6.8, §2.9.2): the titles. FestOfHonor reads an Honor's edition and slot (a template in
// its edition's TitleIds: the Main, a championship's Mini or Micro; a crown Honor's series); FestRoll groups the world's Honors by
// edition, newest first, only what happened in this world (Blank and Established alike); FestDefending is the latest roll entry of its
// kind from an edition before the brand's current or next one, from its Honor until the next edition crowns (never crowned: two days
// past its last event, R-13); IsDefending asks every brand's Main; TitleRank, FestTitleBefore and FestTopTitle are the title order
// (class, then prize or a belt's points, then date); FestivalTitle is Discover's and Lasting's term (G/WorldSim.cpp); FestTitlePost
// finds a Champion or Won post's title for World::Headline (G/WorldText.cpp). HeroFestPins walks the hero's results once per world
// state (a pin per edition: outline played, silver cashed, gold won; satellites never count) and HeroFestResults lists one edition's,
// newest first. The rolls and pins are kept in FestViews beside the race (one reader, PC-9). No RNG (§2.12).
#include "ShortStack/Game/WorldFest.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "WorldFestDetail.h"
#include "WorldSim.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace fest
{
/** World::FestViews' contents (H0, LEAD_CALLS R-10): the festival views kept per world state. Never saved; a copied World starts
 *  without them (World::FestViewBox), so a snapshot builds its own on its one reader (the host's writer, which reads none). */
struct WorldViews
{
	/** The race the views read, ranked once per world state: the live race, else the last crown's top ten (the Afterglow). */
	struct RaceView
	{
		bool Valid = false;
		uint64_t Serial = 0; // the world state it was ranked at (WorldStamp: serial, Rev, HeroRev), and the race's key and rows then
		int Rev = -1, HeroRev = -1;
		std::string Key;
		size_t Rows = 0;
		bool Live = false;                                    // the live race (else FestLast's top ten, or nothing)
		int Scorers = 0;                                      // the race's scorers (FestLast.Scorers in the Afterglow)
		std::vector<std::pair<int, world::FestScore>> Order;  // rank order: every scorer live, the top ten after the crown
		std::vector<std::pair<int, int>> RankOf;              // (npc, rank) by npc, for the standing's lookups
		std::vector<std::pair<int, int>> Known;               // FestKnown: (npc, rank), race order
	};
	RaceView Race;
	/** The crown Honors by series id (FestCrownedAt, which fest::PhaseOf reads), kept per world state and the Honors' count. */
	struct CrownView
	{
		bool Valid = false;
		uint64_t Serial = 0;
		int Rev = -1, HeroRev = -1;
		size_t Honors = 0;
		std::vector<std::pair<std::string, double>> At; // (series id, the crown's minute), sorted by id
	};
	CrownView Crowns;
	/** FW-3: the champions' rolls by brand (FestRoll; pointers into the world's Honors), kept per world state and the Honors' count and
	 *  storage, with IsDefending's last answer (the Main defenders on one day). */
	struct RollView
	{
		bool Valid = false;
		uint64_t Serial = 0;
		int Rev = -1, HeroRev = -1;
		size_t Honors = 0;
		const world::Honor* Data = nullptr;
		std::vector<std::vector<world::FestRollRow>> ByBrand; // fest::Brand::Count lists, newest edition first
		int DefDay = INT_MIN;
		std::vector<int> Defenders; // the npcs (-1 the hero) defending a Main on DefDay, sorted
	};
	RollView Rolls;
	/** FW-3: the hero's pins (HeroFestPins), kept per world state and the results' count, and one edition's result rows
	 *  (HeroFestResults: the YOUR panel's, newest first). */
	struct PinsView
	{
		bool Valid = false;
		uint64_t Serial = 0;
		int Rev = -1, HeroRev = -1;
		size_t Results = 0;
		std::vector<world::HeroFestPin> Pins;
	};
	PinsView Pins;
	struct ResultsView
	{
		bool Valid = false;
		uint64_t Serial = 0;
		int Rev = -1, HeroRev = -1;
		size_t Results = 0;
		std::string Key;
		std::vector<world::HeroFestResult> Rows;
	};
	ResultsView Results;
};

/** World's friend for the festival modules (§6.2): the store World keeps for them (F-world's reads and writes from G1). */
struct WorldAccess
{
	static world::FestRace& Race(world::World& W) { return W.Fest; }
	static const world::FestRace& Race(const world::World& W) { return W.Fest; }
	static world::FestFinal& Last(world::World& W) { return W.FestLast; }
	static const world::FestFinal& Last(const world::World& W) { return W.FestLast; }
	static std::vector<world::HeroFestMark>& Marks(world::World& W) { return W.HeroFest; }
	static const std::vector<world::HeroFestMark>& Marks(const world::World& W) { return W.HeroFest; }
	/** The views' cache, made on first use (a display read on the world's one reader thread: mutable, no lock, PLAN_G1 PC-9). */
	static WorldViews& Views(const world::World& W)
	{
		if (!W.FestViews.P)
		{
			W.FestViews.P = std::make_shared<WorldViews>();
		}
		return *W.FestViews.P;
	}
	/** A race writer changed the race: the ranked view is rebuilt on its next read (nothing is made when no view was read). */
	static void DropRace(world::World& W)
	{
		if (W.FestViews.P)
		{
			W.FestViews.P->Race.Valid = false;
		}
	}
	// FW-2b: the crown's reads and writes (§2.6.1).
	/** The world's Honors (the crown pushes its Honor here, the world\thonor line). */
	static std::vector<world::Honor>& Titles(world::World& W) { return W.Titles; }
	/** The planned events, read only (a const read never clones the shared queue). */
	static const world::CowVec<world::Pending>& Queue(const world::World& W) { return W.Queue; }
	/** The world's log, the post the crown just made (Sim::Post) to flag. */
	static world::WorldEvent& LastPost(world::World& W) { return W.Log.back(); }
	/** A person to change, and their row refreshed after (the views' rows read it). */
	static world::Npc& Edit(world::World& W, int Id) { return W.Edit(Id); }
	static void Refresh(world::World& W, int Id) { W.Refresh(Id); }
	/** Something of the player's changed (their marks): the session's next save writes the world. */
	static void HeroChanged(world::World& W) { ++W.HeroRev; }
	// FW-3: the pins' read (§2.6.4).
	/** The player's finished events (World's private HeroResults: event id -> place, prize; never pruned). */
	static const std::map<std::string, std::pair<int, Chips>>& HeroResults(const world::World& W) { return W.HeroResults; }
};
} // namespace fest

namespace world
{
namespace worldfest_detail
{
/** ceil(Share × Scorers) in whole ranks (§2.5.3; FestMarksNested holds it to the whole-number rule for every S to 3,000). */
int FestShareRanks(double Share, int Scorers)
{
	return static_cast<int>(std::ceil(Share * static_cast<double>(Scorers)));
}

/** Whether a crown Honor already names the series (§2.5.2 rule 1: a crowned edition never reopens; its EventId is "<series>#pos"). */
bool FestCrownedKey(const World& W, const std::string& Key)
{
	const std::string Id = Key + "#pos";
	for (const Honor& H : W.Honors())
	{
		if (H.EventId == Id)
		{
			return true;
		}
	}
	return false;
}

/** One finish into the live race (§2.5.2 rules 2-3): online, the live key's series, not a satellite, a place; a row once it scores. */
void FestAddFinish(World& W, const Pending& P, int Npc, int Place)
{
	FestRace& Race = fest::WorldAccess::Race(W);
	if (Race.Key.empty() || !P.Online || P.Series != Race.Key || Place < 1 || (P.Kinds & KindSatellite) != 0)
	{
		return;
	}
	const double Pts = fest::RacePoints(Place, P.Entries, P.BuyIn, Sim::CurveOf(P));
	if (!(Pts > 0.0))
	{
		return;
	}
	FestScore& Row = Race.Scores[Npc];
	Row.Points += Pts;
	++Row.Cashes;
	fest::WorldAccess::DropRace(W);
}

/** The ranked race the views read, rebuilt when the world's stamp moved or a writer dropped it (§2.6.6: cached per world Rev). */
const fest::WorldViews::RaceView& FestRaceView(const World& W)
{
	fest::WorldViews::RaceView& V = fest::WorldAccess::Views(W).Race;
	const WorldStamp Stamp = W.SnapshotStamp();
	const FestRace& Race = fest::WorldAccess::Race(W);
	const FestFinal& Last = fest::WorldAccess::Last(W);
	const std::string& Key = !Race.Key.empty() ? Race.Key : Last.Key;
	const size_t Rows = !Race.Key.empty() ? Race.Scores.size() : Last.Top.size();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Rows == Rows && V.Key == Key)
	{
		return V;
	}
	V = fest::WorldViews::RaceView();
	if (!Race.Key.empty())
	{
		V.Live = true;
		V.Order = FestRank(Race);
		V.Scorers = static_cast<int>(V.Order.size());
	}
	else if (!Last.Key.empty())
	{
		V.Order = Last.Top;
		V.Scorers = Last.Scorers;
	}
	V.RankOf.reserve(V.Order.size());
	const std::map<int, Bond>& Bonds = W.Bonds();
	for (size_t K = 0; K < V.Order.size(); ++K)
	{
		const int Id = V.Order[K].first;
		const int Rank = static_cast<int>(K) + 1;
		V.RankOf.emplace_back(Id, Rank);
		if (Id < 0)
		{
			continue;
		}
		// PEOPLE YOU KNOW (§2.5.4): the player's rival, or a bond holding a memory.
		const Npc* Person = W.Get(Id);
		const auto Held = Bonds.find(Id);
		if ((Person && Person->Rival) || (Held != Bonds.end() && !Held->second.Memories.empty()))
		{
			V.Known.emplace_back(Id, Rank);
		}
	}
	std::sort(V.RankOf.begin(), V.RankOf.end());
	V.Key = Key;
	V.Rows = Rows;
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Valid = true;
	return V;
}

/** A scorer's rank in the view (0: not in it). */
int FestRankIn(const fest::WorldViews::RaceView& V, int Npc)
{
	const auto It = std::lower_bound(V.RankOf.begin(), V.RankOf.end(), std::make_pair(Npc, 0));
	return It != V.RankOf.end() && It->first == Npc ? It->second : 0;
}

/** The crown Honors by series id, rebuilt when the world's stamp or the Honors' count moved (the crown pushes one; a load moves the
 *  serial). */
const fest::WorldViews::CrownView& FestCrownView(const World& W)
{
	fest::WorldViews::CrownView& V = fest::WorldAccess::Views(W).Crowns;
	const WorldStamp Stamp = W.SnapshotStamp();
	const std::vector<Honor>& Honors = W.Honors();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Honors == Honors.size())
	{
		return V;
	}
	V = fest::WorldViews::CrownView();
	for (const Honor& H : Honors)
	{
		if (IsCrownHonor(H))
		{
			V.At.emplace_back(H.EventId.substr(0, H.EventId.size() - 4), H.At); // "<series>#pos"
		}
	}
	// One crown a key (a crowned key never reopens); were there two, the first stands.
	std::stable_sort(V.At.begin(), V.At.end(), [](const std::pair<std::string, double>& A, const std::pair<std::string, double>& B) { return A.first < B.first; });
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Honors = Honors.size();
	V.Valid = true;
	return V;
}

/**
 * The crown of F's race at Now (§2.6.1 steps 1-7; deterministic, no RNG): rank it (S scorers; none, or a key already crowned: the
 * race closes with no champion); the Player of the Series Honor; the FestivalCrown post with the site's bits (and FlagHero for the
 * hero's); +6 online fame to a person who wins (half the Player of the Year's 12); the hero's mark, if they scored, with its crown
 * ticket from FINALIST up while fest::kCrownTickets; FestLast with the top ten; the race closed.
 */
void FestCrown(World& W, const fest::Festival& F, double Now)
{
	FestRace& Race = fest::WorldAccess::Race(W);
	const std::vector<std::pair<int, FestScore>> Order = FestRank(Race);
	const int Scorers = static_cast<int>(Order.size());
	const std::string Key = Race.Key;
	Race = FestRace();
	fest::WorldAccess::DropRace(W);
	if (Scorers == 0 || FestCrownedKey(W, Key))
	{
		return;
	}
	// 2. The Honor (the world\thonor line; Entries holds the points, the Player of the Year precedent).
	const int Winner = Order.front().first;
	const Npc* Person = Winner >= 0 ? W.Get(Winner) : nullptr;
	const int Points = static_cast<int>(std::llround(Order.front().second.Points));
	Honor H;
	H.Year = YearOf(F.FirstDay);
	H.Title = F.Name + " Player of the Series";
	H.EventId = Key + "#pos";
	H.Npc = Winner;
	H.Name = Person ? Person->Name : Winner < 0 ? W.HeroName : std::string();
	H.Prize = 0;
	H.Entries = Points;
	H.At = Now;
	fest::WorldAccess::Titles(W).push_back(H);
	// 3. The post: the site's bits, so a rival's crown shows on its own News and stays off RiverLine's.
	Sim::Post(W, Now, EventKind::FestivalCrown, Winner, F.Name, static_cast<Chips>(Order.front().second.Cashes), Points);
	fest::WorldAccess::LastPost(W).Flags |= Sim::SiteFlags(F.Site) | (Winner < 0 ? FlagHero : 0);
	// 4. A person's fame.
	if (Person)
	{
		Npc& N = fest::WorldAccess::Edit(W, Winner);
		N.Fame[static_cast<size_t>(Rep::Online)] = static_cast<float>(static_cast<double>(N.Fame[static_cast<size_t>(Rep::Online)]) + 6.0);
		fest::WorldAccess::Refresh(W, Winner);
	}
	// 5 and 7. The hero's mark, and the crown ticket it earns (paid once by the session's FestStep).
	for (size_t K = 0; K < Order.size(); ++K)
	{
		if (Order[K].first != -1)
		{
			continue;
		}
		HeroFestMark M;
		M.Key = Key;
		M.Rank = static_cast<int>(K) + 1;
		M.Scorers = Scorers;
		M.Mark = MarkFor(M.Rank, Scorers);
		M.Points = Order[K].second.Points;
		M.Ticket = fest::kCrownTickets && M.Mark >= FestMark::Finalist;
		fest::WorldAccess::Marks(W).push_back(M);
		fest::WorldAccess::HeroChanged(W);
		break;
	}
	// 6. The last crown, through the Afterglow: the top ten in rank order.
	FestFinal& Last = fest::WorldAccess::Last(W);
	Last = FestFinal();
	Last.Key = Key;
	Last.At = Now;
	Last.Scorers = Scorers;
	Last.Top.assign(Order.begin(), Order.begin() + std::min<std::ptrdiff_t>(10, static_cast<std::ptrdiff_t>(Order.size())));
	fest::WorldAccess::DropRace(W);
}

// ---- FW-3: the titles, the roll, the defender and the pins (§2.6.2-§2.6.5)

/** Whether S ends in Tail. */
bool FestEndsWith(const std::string& S, const char* Tail)
{
	const std::string T(Tail);
	return S.size() >= T.size() && S.compare(S.size() - T.size(), T.size(), T) == 0;
}

/** An event id's template id ("<template>@<minute>" -> "<template>"; the id itself when it has no '@'). */
std::string FestTemplateOfId(const std::string& EventId)
{
	return EventId.substr(0, EventId.rfind('@'));
}

/** The editions of brand B in fest::All()'s FirstDay order. */
std::vector<const fest::Festival*> FestEditionsOf(fest::Brand B)
{
	std::vector<const fest::Festival*> Out;
	for (const fest::Festival& F : fest::All())
	{
		if (F.Which == B)
		{
			Out.push_back(&F);
		}
	}
	return Out;
}

/** The champions' rolls by brand, rebuilt when the world's stamp, the Honors' count or their storage moved (§2.6.6: cached per world
 *  Rev). Rows by edition, newest first; a slot's first Honor stands (an edition has one Main, one Mini, one Micro and one crown). */
fest::WorldViews::RollView& FestRollView(const World& W)
{
	fest::WorldViews::RollView& V = fest::WorldAccess::Views(W).Rolls;
	const WorldStamp Stamp = W.SnapshotStamp();
	const std::vector<Honor>& Honors = W.Honors();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Honors == Honors.size() && V.Data == Honors.data())
	{
		return V;
	}
	V = fest::WorldViews::RollView();
	V.ByBrand.resize(static_cast<size_t>(fest::Brand::Count));
	std::map<const fest::Festival*, FestRollRow> Rows;
	for (const Honor& H : Honors)
	{
		FestSlot Slot = FestSlot::None;
		const fest::Festival* F = FestOfHonor(H, Slot);
		if (!F)
		{
			continue;
		}
		FestRollRow& R = Rows[F];
		R.Key = F->SeriesId;
		R.Year = F->Year;
		R.Edition = F->Edition;
		const Honor** Into = Slot == FestSlot::Main ? &R.Main : Slot == FestSlot::Mini ? &R.Mini : Slot == FestSlot::Micro ? &R.Micro : &R.Crown;
		*Into = *Into ? *Into : &H;
	}
	for (const std::pair<const fest::Festival* const, FestRollRow>& It : Rows)
	{
		V.ByBrand[static_cast<size_t>(It.first->Which)].push_back(It.second);
	}
	for (std::vector<FestRollRow>& List : V.ByBrand)
	{
		// Newest edition first (a brand's editions never share a year; the series id breaks a tie all the same).
		std::sort(List.begin(), List.end(), [](const FestRollRow& A, const FestRollRow& B) { return A.Year != B.Year ? A.Year > B.Year : A.Key > B.Key; });
	}
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Honors = Honors.size();
	V.Data = Honors.data();
	V.Valid = true;
	return V;
}

/** Whether edition F's reign is over on Day (§2.6.3: a defender holds until the brand's next edition crowns): F crowned on or before
 *  Day; never crowned (no race, no scorer, an old save), two days past its last event (R-13's Off). */
bool FestEditionGone(const World& W, const fest::Festival& F, int Day)
{
	const double Crowned = FestCrownedAt(W, F.SeriesId);
	if (Crowned >= 0.0)
	{
		return sim::DayAt(Crowned) <= Day;
	}
	return static_cast<double>(Day) * 1440.0 >= fest::LastEnd(F) + 2.0 * 1440.0;
}

/** One festival result of the hero's (HeroFestPins' and HeroFestResults' walk). */
struct FestHeroRow
{
	const fest::Festival* F = nullptr;
	const std::string* EventId = nullptr;
	int Place = 0;
	Chips Prize = 0;
	double Start = 0.0;
};

/** The hero's festival results (§2.6.4): every finished event whose template is a festival series' (numbered, headliners, Main), never
 *  a satellite. HeroResults is sorted by event id, so one template's instances are adjacent: one template lookup a template. */
std::vector<FestHeroRow> FestHeroWalk(const World& W)
{
	std::vector<FestHeroRow> Out;
	std::string LastTemplate;
	const fest::Festival* LastF = nullptr;
	bool Seen = false;
	for (const std::pair<const std::string, std::pair<int, Chips>>& It : fest::WorldAccess::HeroResults(W))
	{
		const size_t At = It.first.rfind('@');
		if (At == std::string::npos)
		{
			continue;
		}
		if (!Seen || It.first.compare(0, At, LastTemplate) != 0 || LastTemplate.size() != At)
		{
			Seen = true;
			LastTemplate.assign(It.first, 0, At);
			const net::EventTemplate* T = net::ForEvent(It.first).FindTemplate(LastTemplate);
			LastF = T && T->Fmt != net::Format::Satellite && !T->Series.empty() ? fest::FestivalOf(T->Series) : nullptr;
		}
		if (LastF)
		{
			FestHeroRow R;
			R.F = LastF;
			R.EventId = &It.first;
			R.Place = It.second.first;
			R.Prize = It.second.second;
			R.Start = std::atof(It.first.c_str() + At + 1);
			Out.push_back(R);
		}
	}
	return Out;
}

/** An instance's field (FindInstance on its site's network), 0 when it can't be found. */
int FestEntriesOf(const std::string& EventId)
{
	net::EventInstance E;
	return net::ForEvent(EventId).FindInstance(EventId, E) ? E.Entries : 0;
}
} // namespace worldfest_detail

// ---- the marks (§2.5.3; G/WorldFestDetail.h)

int FestFinalistRanks(int Scorers)
{
	if (Scorers <= 0)
	{
		return 0;
	}
	if (Scorers >= fest::kFinalistSmallBelow)
	{
		return std::clamp(worldfest_detail::FestShareRanks(fest::kTop10, Scorers), 3, fest::kFinalistCap);
	}
	return std::clamp(worldfest_detail::FestShareRanks(fest::kFinalistSmallShare, Scorers), 3, fest::kFinalistSmall);
}

FestMark MarkFor(int Rank, int Scorers)
{
	if (Rank < 1 || Rank > Scorers)
	{
		return FestMark::None;
	}
	if (Rank == 1)
	{
		return FestMark::Title;
	}
	if (Rank <= 3)
	{
		return FestMark::Podium;
	}
	if (Rank <= FestFinalistRanks(Scorers))
	{
		return FestMark::Finalist;
	}
	if (Rank <= worldfest_detail::FestShareRanks(fest::kTop10, Scorers))
	{
		return FestMark::Top10;
	}
	if (Rank <= worldfest_detail::FestShareRanks(fest::kTop25, Scorers))
	{
		return FestMark::Top25;
	}
	return FestMark::None;
}

int FestLastRank(FestMark Mark, int Scorers)
{
	if (Scorers <= 0 || Mark <= FestMark::None || Mark >= FestMark::Count)
	{
		return 0;
	}
	// The widest rank among the grades at or above Mark (MarkFor takes the best grade a rank reaches).
	int Last = 1;
	if (Mark <= FestMark::Podium)
	{
		Last = std::max(Last, 3);
	}
	if (Mark <= FestMark::Finalist)
	{
		Last = std::max(Last, FestFinalistRanks(Scorers));
	}
	if (Mark <= FestMark::Top10)
	{
		Last = std::max(Last, worldfest_detail::FestShareRanks(fest::kTop10, Scorers));
	}
	if (Mark <= FestMark::Top25)
	{
		Last = std::max(Last, worldfest_detail::FestShareRanks(fest::kTop25, Scorers));
	}
	return std::min(Last, Scorers);
}

int FestPointsTo(const World& W, FestMark Mark)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const int Hero = V.Live ? worldfest_detail::FestRankIn(V, -1) : 0;
	const int Last = V.Live ? FestLastRank(Mark, V.Scorers) : 0;
	if (Hero == 0 || Last == 0)
	{
		return -1; // no live race, no such mark, or no cash yet: CASH 1 is the lit rung, with no number
	}
	if (Hero <= Last)
	{
		return 0;
	}
	const double Cutoff = V.Order[static_cast<size_t>(Last - 1)].second.Points;
	const double Mine = V.Order[static_cast<size_t>(Hero - 1)].second.Points;
	return static_cast<int>(std::llround(Cutoff - Mine)) + 1;
}

// ---- the race and the crown (§2.5.2, §2.6.1)

void FestDay(World& W, int Today)
{
	for (const fest::Festival* F : fest::Festivals(Today, Today))
	{
		const double Launched = W.SiteLaunched(F->Site);
		if (F->FirstDay != Today || Launched < 0.0 || Launched > W.Clock() || worldfest_detail::FestCrownedKey(W, F->SeriesId))
		{
			continue;
		}
		FestRace& Race = fest::WorldAccess::Race(W);
		if (Race.Key == F->SeriesId)
		{
			continue; // open already
		}
		// A stale key (an earlier festival's race still open: impossible under the calendar, FestHour crowns each race within two days
		// of its last event) is crowned first (§2.5.2 rule 1), now; a key that names no festival just closes.
		if (!Race.Key.empty())
		{
			const fest::Festival* Stale = fest::FestivalOf(Race.Key);
			if (Stale)
			{
				worldfest_detail::FestCrown(W, *Stale, W.Clock());
			}
		}
		Race = FestRace();
		Race.Key = F->SeriesId;
		fest::WorldAccess::DropRace(W);
	}
}

void FestHour(World& W, double Now)
{
	// The Afterglow ends three days after the crown (§2.6.1): the last crown's top ten are gone.
	FestFinal& Last = fest::WorldAccess::Last(W);
	if (!Last.Key.empty() && Now >= Last.At + static_cast<double>(fest::kAfterglowDays) * 1440.0)
	{
		Last = FestFinal();
		fest::WorldAccess::DropRace(W);
	}
	// The crown (§2.6.1): the live race's festival past its last event's scheduled end (a compare an hour), and no event of its series
	// left in the queue (the scan only from then: one the hero is still seated in waits for them).
	const std::string& Key = fest::WorldAccess::Race(W).Key;
	if (Key.empty())
	{
		return;
	}
	const fest::Festival* F = fest::FestivalOf(Key);
	if (!F || Now < fest::LastEnd(*F))
	{
		return;
	}
	const World& Read = W;
	for (const Pending& P : fest::WorldAccess::Queue(Read))
	{
		if (P.Series == Key)
		{
			return;
		}
	}
	worldfest_detail::FestCrown(W, *F, Now);
}

void FestScoreResult(World& W, const Pending& P, int Npc, int Place)
{
	worldfest_detail::FestAddFinish(W, P, Npc, Place);
}

void FestScoreHero(World& W, const Pending& Ev, int Place)
{
	// The hero's row is -1; World::HeroFinished calls this once per result (inside !Again: a result told twice is one result).
	worldfest_detail::FestAddFinish(W, Ev, -1, Place);
}

std::vector<std::pair<int, FestScore>> FestRank(const FestRace& Race)
{
	std::vector<std::pair<int, FestScore>> Order(Race.Scores.begin(), Race.Scores.end());
	// A total order (§2.12): points descending; on a tie a person ahead of the hero (-1: the hero must be strictly ahead, the Player
	// of the Year precedent), then the lower npc id.
	std::sort(Order.begin(), Order.end(), [](const std::pair<int, FestScore>& A, const std::pair<int, FestScore>& B) {
		if (A.second.Points != B.second.Points)
		{
			return A.second.Points > B.second.Points;
		}
		if ((A.first < 0) != (B.first < 0))
		{
			return B.first < 0;
		}
		return A.first < B.first;
	});
	return Order;
}

// ---- the views (§2.6.6)

std::vector<std::pair<int, FestScore>> FestTop(const World& W, int Count)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const size_t N = std::min(V.Order.size(), static_cast<size_t>(std::max(0, Count)));
	return std::vector<std::pair<int, FestScore>>(V.Order.begin(), V.Order.begin() + static_cast<std::ptrdiff_t>(N));
}

std::pair<int, int> FestStanding(const World& W, int Npc)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const int Rank = worldfest_detail::FestRankIn(V, Npc);
	if (Rank == 0 && !V.Live && Npc < 0)
	{
		// After the crown the hero's place outside the top ten is their mark's (FW-2b writes it at the crown).
		const std::string& Key = fest::WorldAccess::Last(W).Key;
		for (const HeroFestMark& M : fest::WorldAccess::Marks(W))
		{
			if (!Key.empty() && M.Key == Key)
			{
				return {M.Rank, V.Scorers};
			}
		}
	}
	return {Rank, V.Scorers};
}

std::vector<std::pair<int, int>> FestKnown(const World& W)
{
	return worldfest_detail::FestRaceView(W).Known;
}

double FestCrownedAt(const World& W, const std::string& Key)
{
	// The crown Honor's minute (fest::PhaseOf's Closing and Afterglow, §2.1.4); -1: never crowned (no race, no scorer, an old save).
	const fest::WorldViews::CrownView& V = worldfest_detail::FestCrownView(W);
	const auto It = std::lower_bound(V.At.begin(), V.At.end(), Key, [](const std::pair<std::string, double>& A, const std::string& K) { return A.first < K; });
	return It != V.At.end() && It->first == Key ? It->second : -1.0;
}

std::vector<FestRollRow> FestRoll(const World& W, fest::Brand B)
{
	// The brand's titles and belts as this world made them (§2.6.2): Honors whose template is in an edition's TitleIds, and crown Honors,
	// by edition, newest first. Honors come only from resolved events, so a Blank and an Established world read alike (U5).
	if (B <= fest::Brand::None || B >= fest::Brand::Count)
	{
		return {};
	}
	return worldfest_detail::FestRollView(W).ByBrand[static_cast<size_t>(B)];
}

const Honor* FestDefending(const World& W, fest::Brand B, bool Crown, int Day)
{
	// §2.6.3: the latest roll entry of the kind (the Main's champion, or the belt) from an edition before the brand's current or next
	// one (the first whose window has not closed by Day), valid from its Honor until the brand's next edition after it crowns.
	if (B <= fest::Brand::None || B >= fest::Brand::Count)
	{
		return nullptr;
	}
	const std::vector<const fest::Festival*> Editions = worldfest_detail::FestEditionsOf(B);
	int Current = static_cast<int>(Editions.size());
	for (int K = static_cast<int>(Editions.size()) - 1; K >= 0; --K)
	{
		Current = Editions[static_cast<size_t>(K)]->LastDay >= Day ? K : Current;
	}
	for (const FestRollRow& R : worldfest_detail::FestRollView(W).ByBrand[static_cast<size_t>(B)])
	{
		const Honor* H = Crown ? R.Crown : R.Main;
		int Of = -1;
		for (int K = 0; K < static_cast<int>(Editions.size()); ++K)
		{
			Of = Editions[static_cast<size_t>(K)]->SeriesId == R.Key ? K : Of;
		}
		if (!H || Of < 0 || Of >= Current || sim::DayAt(H->At) > Day)
		{
			continue;
		}
		const bool Gone = Of + 1 < static_cast<int>(Editions.size()) && worldfest_detail::FestEditionGone(W, *Editions[static_cast<size_t>(Of + 1)], Day);
		return Gone ? nullptr : H;
	}
	return nullptr;
}

bool IsDefending(const World& W, int Npc, int Day)
{
	// DEFENDING CHAMPION keys on a Main (§2.6.3; a championship's too, from its edition II): any brand's defender on Day, kept for the
	// last day asked (the plate and the pills ask it for every name they draw).
	fest::WorldViews::RollView& V = worldfest_detail::FestRollView(W);
	if (V.DefDay != Day)
	{
		V.Defenders.clear();
		for (int B = static_cast<int>(fest::Brand::None) + 1; B < static_cast<int>(fest::Brand::Count); ++B)
		{
			if (const Honor* H = FestDefending(W, static_cast<fest::Brand>(B), false, Day))
			{
				V.Defenders.push_back(H->Npc);
			}
		}
		std::sort(V.Defenders.begin(), V.Defenders.end());
		V.DefDay = Day;
	}
	return Npc >= -1 && std::binary_search(V.Defenders.begin(), V.Defenders.end(), Npc);
}

bool IsCrownHonor(const Honor& H)
{
	// The Player of the Series belt (§2.6.1 step 2: EventId = the series id + "#pos"). No event id ends so: they are
	// "<template>@<minute>", "roty#<year>" from G2.
	static const std::string FestPosTail = "#pos";
	return H.EventId.size() >= FestPosTail.size() && H.EventId.compare(H.EventId.size() - FestPosTail.size(), FestPosTail.size(), FestPosTail) == 0;
}

int TitleRank(const Honor& H)
{
	// The title order's class (§2.6.3): 1 a championship's Main, 2 any other festival's Main, 3 a championship's belt, 4 any other belt,
	// 5 a championship's Mini or Micro; 0 not a festival title (a numbered event, a series that is no festival's, the year's titles).
	FestSlot Slot = FestSlot::None;
	const fest::Festival* F = FestOfHonor(H, Slot);
	if (!F)
	{
		return 0;
	}
	const bool Champ = fest::IsChampionship(F->Which);
	switch (Slot)
	{
	case FestSlot::Main:
		return Champ ? 1 : 2;
	case FestSlot::Crown:
		return Champ ? 3 : 4;
	case FestSlot::Mini:
	case FestSlot::Micro:
		return 5;
	default:
		return 0;
	}
}

bool FestivalTitle(const Pending& P)
{
	// v1.6 (§2.6.5): an online event whose template is in its festival's TitleIds (the Main, a championship's Mini and Micro). Discover's
	// Always term and Lasting's (G/WorldSim.cpp) read it for every resolved event, so a title's series is asked first (most events have
	// none).
	return P.Online && !P.Series.empty() && fest::IsTitleTemplate(worldfest_detail::FestTemplateOfId(P.Id));
}

std::vector<HeroFestPin> HeroFestPins(const World& W)
{
	// §2.6.4: a pin per edition the hero played (numbered events, headliners, the Main; never a satellite), derived from HeroResults once
	// per world state: gold won an event, silver cashed, outline played; the best place and its field. Editions in calendar order.
	fest::WorldViews::PinsView& V = fest::WorldAccess::Views(W).Pins;
	const WorldStamp Stamp = W.SnapshotStamp();
	const size_t Count = fest::WorldAccess::HeroResults(W).size();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Results == Count)
	{
		return V.Pins;
	}
	V = fest::WorldViews::PinsView();
	std::map<const fest::Festival*, std::pair<HeroFestPin, const worldfest_detail::FestHeroRow*>> By;
	const std::vector<worldfest_detail::FestHeroRow> Rows = worldfest_detail::FestHeroWalk(W);
	for (const worldfest_detail::FestHeroRow& R : Rows)
	{
		std::pair<HeroFestPin, const worldfest_detail::FestHeroRow*>& Slot = By[R.F];
		HeroFestPin& Pin = Slot.first;
		Pin.Key = R.F->SeriesId;
		++Pin.Events;
		Pin.Cashes += R.Prize > 0 ? 1 : 0;
		Pin.Wins += R.Place == 1 ? 1 : 0;
		// The best place (a tie: the newer event) and, below, its field.
		const worldfest_detail::FestHeroRow* Best = Slot.second;
		if (R.Place >= 1 && (!Best || R.Place < Best->Place || (R.Place == Best->Place && R.Start > Best->Start)))
		{
			Slot.second = &R;
		}
	}
	for (const std::pair<const fest::Festival* const, std::pair<HeroFestPin, const worldfest_detail::FestHeroRow*>>& It : By)
	{
		HeroFestPin Pin = It.second.first;
		Pin.Grade = Pin.Wins > 0 ? 3 : Pin.Cashes > 0 ? 2 : 1;
		if (const worldfest_detail::FestHeroRow* Best = It.second.second)
		{
			Pin.BestPlace = Best->Place;
			Pin.BestEntries = worldfest_detail::FestEntriesOf(*Best->EventId);
		}
		V.Pins.push_back(Pin);
	}
	// (fest::All() is one vector in FirstDay order, so its rows' addresses are calendar order: By's order.)
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Results = Count;
	V.Valid = true;
	return V.Pins;
}

FestMark HeroMark(const World& W, const std::string& Key)
{
	// The hero's mark in edition Key, kept from its crown (§2.5.3; one record an edition scored in): None before it or without one.
	for (const HeroFestMark& M : fest::WorldAccess::Marks(W))
	{
		if (M.Key == Key)
		{
			return M.Mark;
		}
	}
	return FestMark::None;
}

void FestTicketPaid(World& W, const std::string& Key)
{
	for (HeroFestMark& M : fest::WorldAccess::Marks(W))
	{
		if (M.Key == Key && M.Ticket)
		{
			M.Ticket = false;
			fest::WorldAccess::HeroChanged(W);
		}
	}
}

// ---- G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): F-world's bodies (FestRaceKey FW-2a, HeroMarks FW-2b, HeroFestResults
// FW-3); FestWhyLine a stub answering "none" until FW-4.

const std::vector<HeroFestMark>& HeroMarks(const World& W)
{
	// Every mark the hero keeps, crown order (World's HeroFest: the world\tfest\tmark lines).
	return fest::WorldAccess::Marks(W);
}

const std::string& FestRaceKey(const World& W)
{
	// The race FestTop, FestStanding, FestKnown and MarkCutoff read: the live one from its opening, else the last crown's through the
	// Afterglow (FW-2b clears FestLast after it), else none.
	const std::string& Live = fest::WorldAccess::Race(W).Key;
	return !Live.empty() ? Live : fest::WorldAccess::Last(W).Key;
}

std::string FestWhyLine(const World& /*W*/, const StoryPerson& /*Person*/, const std::string& /*EventId*/)
{
	return std::string();
}

std::vector<HeroFestResult> HeroFestResults(const World& W, const std::string& Key)
{
	// YOUR <FESTIVAL>'s result rows (§2.8.2): edition Key's results as HeroFestPins counts them, newest first, each with its field; kept
	// per world state for the last edition asked (the panel draws one).
	fest::WorldViews::ResultsView& V = fest::WorldAccess::Views(W).Results;
	const WorldStamp Stamp = W.SnapshotStamp();
	const size_t Count = fest::WorldAccess::HeroResults(W).size();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Results == Count && V.Key == Key)
	{
		return V.Rows;
	}
	V = fest::WorldViews::ResultsView();
	std::vector<std::pair<double, HeroFestResult>> Found;
	for (const worldfest_detail::FestHeroRow& R : worldfest_detail::FestHeroWalk(W))
	{
		if (R.F->SeriesId != Key)
		{
			continue;
		}
		HeroFestResult Row;
		Row.EventId = *R.EventId;
		Row.Place = R.Place;
		Row.Prize = R.Prize;
		Row.Entries = worldfest_detail::FestEntriesOf(Row.EventId);
		Found.emplace_back(R.Start, Row);
	}
	std::sort(Found.begin(), Found.end(), [](const std::pair<double, HeroFestResult>& A, const std::pair<double, HeroFestResult>& B) {
		return A.first != B.first ? A.first > B.first : A.second.EventId > B.second.EventId;
	});
	for (std::pair<double, HeroFestResult>& It : Found)
	{
		V.Rows.push_back(std::move(It.second));
	}
	V.Key = Key;
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Results = Count;
	V.Valid = true;
	return V.Rows;
}

// ---- FW-3: G/WorldFestDetail.h's title reads

const fest::Festival* FestOfHonor(const Honor& H, FestSlot& Slot)
{
	Slot = FestSlot::None;
	if (IsCrownHonor(H))
	{
		const fest::Festival* F = fest::FestivalOf(H.EventId.substr(0, H.EventId.size() - 4)); // "<series>#pos"
		Slot = F ? FestSlot::Crown : FestSlot::None;
		return F;
	}
	if (H.EventId.find('@') == std::string::npos)
	{
		return nullptr;
	}
	const std::string Template = worldfest_detail::FestTemplateOfId(H.EventId);
	if (!fest::IsTitleTemplate(Template))
	{
		return nullptr;
	}
	const net::EventTemplate* T = net::ForEvent(H.EventId).FindTemplate(Template);
	const fest::Festival* F = T ? fest::FestivalOf(T->Series) : nullptr;
	if (!F)
	{
		return nullptr;
	}
	// The Main is its MainId; a championship's other two titles end "mini" and "micro" (§2.6.2: "<stem>mini", "<stem>micro").
	Slot = Template == F->MainId ? FestSlot::Main
		: worldfest_detail::FestEndsWith(Template, "mini") ? FestSlot::Mini
		: worldfest_detail::FestEndsWith(Template, "micro") ? FestSlot::Micro
		: FestSlot::None;
	return Slot == FestSlot::None ? nullptr : F;
}

const fest::Festival* FestTitlePost(const World& W, const WorldEvent& E, FestSlot& Slot)
{
	Slot = FestSlot::None;
	if ((E.Kind != EventKind::Champion && E.Kind != EventKind::Won) || (E.Flags & FlagLive) != 0)
	{
		return nullptr;
	}
	// The post's Honor: pushed with the post at the event's end, the winner's, titled with the event's name (Sim::Resolve's Lasting).
	const std::vector<Honor>& Honors = W.Honors();
	for (auto It = Honors.rbegin(); It != Honors.rend(); ++It)
	{
		if (It->At == E.At && It->Npc == E.Npc && It->Title == E.What)
		{
			const fest::Festival* F = FestOfHonor(*It, Slot);
			if (F && Slot != FestSlot::Crown)
			{
				return F;
			}
			Slot = FestSlot::None;
			return nullptr;
		}
	}
	return nullptr;
}

bool FestTitleBefore(const Honor& A, const Honor& B)
{
	// §2.6.3: class first (0, no title, last), then the bigger prize (a belt's points, its Entries), then the newer date.
	const int Ra = TitleRank(A);
	const int Rb = TitleRank(B);
	if (Ra != Rb)
	{
		return Ra != 0 && (Rb == 0 || Ra < Rb);
	}
	const long long Wa = IsCrownHonor(A) ? static_cast<long long>(A.Entries) : static_cast<long long>(A.Prize);
	const long long Wb = IsCrownHonor(B) ? static_cast<long long>(B.Entries) : static_cast<long long>(B.Prize);
	if (Wa != Wb)
	{
		return Wa > Wb;
	}
	return A.At > B.At;
}

const Honor* FestTopTitle(const World& W, int Npc)
{
	const Honor* Top = nullptr;
	for (const Honor& H : W.Honors())
	{
		if (H.Npc == Npc && TitleRank(H) > 0 && (!Top || FestTitleBefore(H, *Top)))
		{
			Top = &H;
		}
	}
	return Top;
}

// ---- WHO'S IN (§4.7.3, PA-27)

const FieldStories& World::FieldStoriesOf(const std::string& /*EventId*/) const
{
	// Phase A: nobody's story is told yet (G1: F-world's People and the Qualifiers group, cached per world revision).
	static const FieldStories FestStoriesNone;
	return FestStoriesNone;
}
} // namespace world

namespace fest
{
double MarkCutoff(const world::World& W, world::FestMark Mark)
{
	// The ladder's cut-off (§2.5.3, U27): the points of the scorer holding the mark's last rank in the live race right now (the
	// ranked view, kept per world state); 0 when no race runs or no scorer holds that rank yet.
	const WorldViews::RaceView& V = world::worldfest_detail::FestRaceView(W);
	const int Last = V.Live ? world::FestLastRank(Mark, V.Scorers) : 0;
	return Last > 0 ? V.Order[static_cast<size_t>(Last - 1)].second.Points : 0.0;
}
} // namespace fest
} // namespace ss
