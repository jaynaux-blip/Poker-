// pro_badges (next_segment_spec §7.6, suite 14): core-badges's tests.
// The edges (club, Verified, the card, the POY reign, the Hall); the priority golden for 3 scopes x 2 frames; the
// anti-clutter rules; privacy; determinism over 400 days; distribution over 3 seeds x 2 years; migration from world v1;
// farming; the words (every line through the copy lint); the packed form and the save; the session's side (the review,
// the celebrations once, the debug grants, Career's progress, the Hall's age hook).
//
//   badges_test            the checks (ctest)
//   badges_test quick      everything but the 2-year distribution runs
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/BadgeArt.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ss;

namespace badges_test
{
int Failures = 0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Check(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

const double NightOne = 1440.0 + 127.0;

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char C : Line)
	{
		if (C == '\t')
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else if (C != '\r')
		{
			Cur += C;
		}
	}
	Out.push_back(Cur);
	return Out;
}

std::string ReadText(const std::string& Path)
{
	std::string Out;
	if (FILE* F = std::fopen(Path.c_str(), "rb"))
	{
		char Buf[65536];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Out.append(Buf, N);
		}
		std::fclose(F);
	}
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

std::string Fixture(const char* Name)
{
	return std::string(SS_TESTS_DIR) + "/fixtures/" + Name;
}

void LoadWorld(world::World& W, const std::string& Text)
{
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		W.Read(Tabs(Line));
	}
	W.Finish();
}

double Seconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

world::Tracker Page(int Events, int Freerolls, double Roi)
{
	// Events paid tournaments at $11 with exactly Roi on them (one big score carries it), then Freerolls that paid $500.
	world::Tracker T;
	const Chips BuyIn = 1100;
	const Chips Profit = static_cast<Chips>(std::llround(Roi * static_cast<double>(BuyIn) * Events));
	for (int K = 0; K < Events; ++K)
	{
		T.Add(BuyIn, K == 0 ? BuyIn * Events + Profit : 0, K == 0 ? 1 : 50, 100, 9, world::TrackStake::Low, world::TrackFormat::Regular);
	}
	for (int K = 0; K < Freerolls; ++K)
	{
		T.Add(0, 50000, 1, 300, 9, world::TrackStake::Micro, world::TrackFormat::Regular);
	}
	return T;
}

world::Honor Titled(const char* Title, int Year, int Npc = 7)
{
	world::Honor H;
	H.Title = Title;
	H.Year = Year;
	H.Npc = Npc;
	H.Entries = 4321;
	return H;
}

/** A holder with everything (the priority golden's). */
badge::Holder Everything(int Day, const std::vector<world::Award>& Awards, const world::Tracker& Stats)
{
	badge::Holder H;
	H.Awards = &Awards;
	H.Stats = &Stats;
	const int Year = world::YearOf(Day);
	H.Honors = {Titled(world::HallOfFameTitle, Year), Titled("Player of the Year", Year - 1), Titled("Live Player of the Year", Year - 1)};
	H.TeamRiverLine = true;
	H.Sponsor = "Volt Rush";
	H.CardBest = world::CardBlack;
	H.VerifiedHeld = true;
	H.KastPartner = true;
	H.WallWins = 2;
	H.WallLast = Day - 3;
	H.WallLastEvent = "Nightly";
	H.ClubVisitsNow = 40;
	H.ClubVisitsAtMonth = 38;
	return H;
}

std::vector<world::Award> Trophies(int Day)
{
	std::vector<world::Award> A(5);
	for (size_t K = 0; K < A.size(); ++K)
	{
		A[K].Day = Day - 400 + static_cast<int>(K) * 30;
		A[K].Ring = K >= 2;
		A[K].Online = false;
		A[K].Main = K == 1;
		A[K].Series = K >= 2 ? "Grand Circuit Montreal 2027" : "The Championship 2027";
		A[K].Event = A[K].Series + ": $1,500 Bounty";
		A[K].Prize = 4500000;
	}
	return A;
}

/** Top, then Top without it, until nothing: the slot order a context and a frame give, as labels. */
std::string Chain(badge::Set S, uint8_t Context, uint8_t Frame)
{
	std::string Out;
	for (int Guard = 0; Guard < 20; ++Guard)
	{
		const badge::Badge* B = badge::Top(S, Context, Frame);
		if (!B)
		{
			break;
		}
		Out += (Out.empty() ? "" : " > ") + badge::Label(*B);
		const badge::Kind K = B->K;
		S.Held.erase(std::remove_if(S.Held.begin(), S.Held.end(), [K](const badge::Badge& X) { return X.K == K; }), S.Held.end());
	}
	return Out;
}

// ------------------------------------------------------------------ the rules at their edges

void Edges()
{
	// The club: 6, 18, 36 in the trailing 90 days; a drop waits for the month's start.
	Expect(badge::ClubTier(5, 0) == 0 && badge::ClubTier(6, 0) == 1 && badge::ClubTier(17, 0) == 1 && badge::ClubTier(18, 0) == 2 && badge::ClubTier(35, 0) == 2 &&
			   badge::ClubTier(36, 0) == 3,
		"club: 5/6, 17/18, 35/36");
	Expect(badge::ClubTier(4, 18) == 2 && badge::ClubTier(20, 6) == 2 && badge::ClubTier(0, 36) == 3, "club: the tier is the better of now's and the 1st's (upgrades at once, drops on the 1st)");
	{
		const int Day = 500;
		std::vector<int> Days = {Day, Day, Day - 1, Day - 89, Day - 90, Day + 1};
		Expect(badge::VisitsIn(Days, Day) == 3, "visits: a day counts once; the window is Day-90 < d <= Day");
		Expect(badge::VisitsIn(Days, Day + 1) == 3 && badge::VisitsIn({}, Day) == 0, "visits: the window moves with the day");
	}
	// Verified Pro: 1,000 paid tournaments, +15% (and so 150 buy-ins up), a tournament in the last 30 days, a professional.
	{
		const world::Tracker Ok = Page(1000, 0, 0.15);
		badge::Holder H;
		H.Stats = &Ok;
		H.Professional = true;
		H.DaysSincePlayed = 29;
		Expect(badge::VerifiedNow(H), "Verified: 1,000 at +15% (150 up), played 29 days ago");
		H.DaysSincePlayed = 30;
		Expect(badge::VerifiedNow(H), "Verified: played 30 days ago still counts");
		H.DaysSincePlayed = 31;
		Expect(!badge::VerifiedNow(H), "Verified: not 31 days after the last tournament");
		H.DaysSincePlayed = 0;
		H.Professional = false;
		Expect(!badge::VerifiedNow(H), "Verified: not with a day job");
		const world::Tracker Short = Page(999, 0, 0.15);
		const world::Tracker Thin = Page(1000, 0, 0.149);
		badge::Holder A = H;
		A.Professional = true;
		A.Stats = &Short;
		Expect(!badge::VerifiedNow(A), "Verified: not at 999 tournaments");
		A.Stats = &Thin;
		Expect(!badge::VerifiedNow(A) && Thin.PaidProfit() / Thin.PaidAbi() == 149, "Verified: not at +14.9%, 149 buy-ins up");
		badge::Holder None;
		Expect(!badge::VerifiedNow(None), "Verified: no record, no badge");
	}
	// The circuit card's standings: from week 13, the top 200 / 50 / 10.
	Expect(badge::CardFor(200, 13) == world::CardRegular && badge::CardFor(201, 13) == world::CardNone && badge::CardFor(1, 12) == world::CardNone &&
			   badge::CardFor(10, 13) == world::CardBlack && badge::CardFor(11, 13) == world::CardGold && badge::CardFor(50, 30) == world::CardGold &&
			   badge::CardFor(51, 30) == world::CardRegular && badge::CardFor(0, 30) == world::CardNone,
		"card: week 12/13, rank 200/201, 50/51, 10/11");
	Expect(world::SeasonWeek(world::DayOn(2027, 3, 25)) == 12 && world::SeasonWeek(world::DayOn(2027, 3, 26)) == badge::CardFromWeek, "card: week 13 is March 26");
	// A card held: the best this season, or last season's through this one.
	{
		const int Day = world::DayOn(2027, 6, 1);
		badge::Holder H;
		H.CardLast = world::CardGold;
		badge::Set S = badge::Earned(H, Day);
		const badge::Badge* C = S.Find(badge::Kind::CircuitCard);
		Expect(C && C->Tier == 2 && C->Year == 2026 && badge::Label(*C) == "GOLD CARD '26", "card: last season's Gold, good through this one");
		H.CardBest = world::CardRegular;
		const badge::Set S2 = badge::Earned(H, Day);
		C = S2.Find(badge::Kind::CircuitCard);
		Expect(C && C->Tier == 2 && C->Year == 2026, "card: the better of this season's and last season's");
		H.CardBest = world::CardBlack;
		const badge::Set S3 = badge::Earned(H, Day);
		C = S3.Find(badge::Kind::CircuitCard);
		Expect(C && C->Tier == 3 && C->Year == 2027 && badge::Label(*C) == "BLACK CARD '27", "card: this season's Black");
	}
	// The Player of the Year reigns the year after: in rows then, on the card forever.
	{
		badge::Holder H;
		H.Honors = {Titled("Player of the Year", 2026)};
		const badge::Set Reigning = badge::Earned(H, world::DayOn(2027, 4, 2));
		const badge::Set Past = badge::Earned(H, world::DayOn(2028, 1, 2));
		const badge::Badge* R = badge::Top(Reigning, badge::Network, badge::FrameNone);
		Expect(R && R->K == badge::Kind::PlayerOfYear && R->Tier == 1 && badge::Label(*R) == "POY '26", "POY: reigning in 2027, in rows");
		Expect(!badge::Top(Past, badge::Network, badge::FrameNone) && Past.Find(badge::Kind::PlayerOfYear) && badge::Top(Past, badge::Card, badge::FrameNone),
			"POY: in 2028 off the rows, still on the card");
		H.Honors.push_back(Titled("Live Player of the Year", 2027));
		const badge::Set Both = badge::Earned(H, world::DayOn(2028, 1, 2));
		const badge::Badge* L = badge::Top(Both, badge::Network, badge::FrameNone);
		Expect(L && L->K == badge::Kind::LivePlayerOfYear && badge::Label(*L) == "LIVE POY '27", "POY: the live title reigns in its own row");
		Expect(!badge::Top(Both, badge::Room, badge::FrameNone), "POY: never on a seat in the room (§5.4's room order)");
	}
	// Bracelets and rings: how many, a Main Event's, the newest one's year.
	{
		const int Day = world::DayOn(2028, 3, 1);
		const std::vector<world::Award> A = Trophies(Day);
		badge::Holder H;
		H.Awards = &A;
		const badge::Set S = badge::Earned(H, Day);
		const badge::Badge* Br = S.Find(badge::Kind::Bracelet);
		const badge::Badge* Rg = S.Find(badge::Kind::Ring);
		Expect(Br && Br->Count == 2 && Br->Main && Rg && Rg->Count == 3 && !Rg->Main && Rg->Color != 0 && Br->Color != 0, "trophies: 2 bracelets (a Main), 3 rings");
		Expect(badge::Label(*Br) == "2\xC3\x97 BRACELET" && badge::Label(*Rg) == "3\xC3\x97 RING", "trophies: the labels count them");
	}
	// The wall and the club from the room's record.
	{
		badge::Holder H;
		H.WallWins = 1;
		H.ClubVisitsNow = 17;
		H.ClubVisitsAtMonth = 18;
		const badge::Set S = badge::Earned(H, 900);
		const badge::Badge* Club = S.Find(badge::Kind::Club);
		Expect(S.Find(badge::Kind::OnTheWall) && Club && Club->Tier == 2 && Club->Count == 17, "room: the wall, and Ember held from the 1st with 17 now");
		Expect(badge::HowEarned(*Club, H).find("holds until the 1st") != std::string::npos, "room: the hover says the card holds until the 1st");
	}
	// The Hall: the class it was inducted with.
	{
		badge::Holder H;
		H.Honors = {Titled(world::HallOfFameTitle, 2036)};
		const badge::Set S = badge::Earned(H, world::DayOn(2040, 1, 1));
		const badge::Badge* B = S.Find(badge::Kind::HallOfFame);
		Expect(B && B->Year == 2036 && badge::Title(*B) == "Hall of Fame, class of 2036", "Hall: the class year");
	}
	// Sponsored never reads the site's own patch (§1.9 #21); Team RiverLine is its own badge.
	{
		badge::Holder H;
		H.Sponsor = "RiverLine";
		Expect(badge::Earned(H, 900).Held.empty(), "sponsored: the RiverLine brand is never 'Sponsored'");
		H.Sponsor = "Volt Rush";
		const badge::Set Volt = badge::Earned(H, 900);
		const badge::Badge* B = Volt.Find(badge::Kind::Sponsored);
		Expect(B && B->Brand == "voltrush" && B->Color == sponsor::FindBrand("voltrush")->Color && badge::Label(*B) == "VOLT RUSH", "sponsored: the brand by id and colour");
		H.Sponsor = "A Brand Nobody Knows";
		const badge::Set Unknown = badge::Earned(H, 900);
		B = Unknown.Find(badge::Kind::Sponsored);
		Expect(B && B->Brand == "A Brand Nobody Knows", "sponsored: a name the catalog doesn't know is kept as written");
	}
}

// ------------------------------------------------------------------ slot orders and the anti-clutter rules

void Priority()
{
	const int Day = world::DayOn(2027, 5, 10);
	const std::vector<world::Award> A = Trophies(Day);
	const world::Tracker T = Page(1200, 0, 0.3);
	const badge::Set S = badge::Earned(Everything(Day, A, T), Day);
	Expect(S.Held.size() == static_cast<size_t>(badge::Kind::Count), "priority: the full set holds all twelve kinds");
	// The golden (§5.4): 3 scopes x 2 frames, each as the chain of tops.
	struct Row
	{
		uint8_t Context;
		uint8_t Frame;
		const char* Want;
	};
	const Row Golden[] = {
		{badge::Network, badge::FrameNone,
			"HALL OF FAME > 2\xC3\x97 BRACELET > 3\xC3\x97 RING > POY '26 > LIVE POY '26 > TEAM RIVERLINE > VOLT RUSH > BLACK CARD '27 > VERIFIED PRO > KAST PARTNER"},
		{badge::Network, badge::FrameChampion, "HALL OF FAME > POY '26 > LIVE POY '26 > TEAM RIVERLINE > VOLT RUSH > BLACK CARD '27 > VERIFIED PRO > KAST PARTNER"},
		{badge::Room, badge::FrameNone,
			"HALL OF FAME > 2\xC3\x97 BRACELET > 3\xC3\x97 RING > ON THE WALL \xC3\x97" "2 > CREST > BLACK CARD '27 > TEAM RIVERLINE > VOLT RUSH > VERIFIED PRO"},
		{badge::Room, badge::FrameChampion, "HALL OF FAME > ON THE WALL \xC3\x97" "2 > CREST > BLACK CARD '27 > TEAM RIVERLINE > VOLT RUSH > VERIFIED PRO"},
		{badge::Stream, badge::FrameNone, "KAST PARTNER > TEAM RIVERLINE > VOLT RUSH > HALL OF FAME > 2\xC3\x97 BRACELET > 3\xC3\x97 RING"},
		{badge::Stream, badge::FrameChampion, "KAST PARTNER > TEAM RIVERLINE > VOLT RUSH > HALL OF FAME"},
	};
	for (const Row& R : Golden)
	{
		const std::string Got = Chain(S, R.Context, R.Frame);
		Check(Got == R.Want, "priority golden (context " + std::to_string(R.Context) + ", frame " + std::to_string(R.Frame) + "): got \"" + Got + "\"");
	}
	// C2: the gold frame says Team RiverLine; a champion frame with no bracelet says the ring.
	Check(Chain(S, badge::Network, badge::FrameGold).find("TEAM RIVERLINE") == std::string::npos, "C2: the gold frame suppresses Team RiverLine");
	{
		badge::Set NoBracelet = S;
		NoBracelet.Held.erase(std::remove_if(NoBracelet.Held.begin(), NoBracelet.Held.end(), [](const badge::Badge& B) { return B.K == badge::Kind::Bracelet; }), NoBracelet.Held.end());
		Check(Chain(NoBracelet, badge::Network, badge::FrameChampion).find("RING") == std::string::npos, "C2: a ring winner's frame suppresses the ring");
	}
	// The card: everything held, in the network's order, then the room's own.
	{
		const std::vector<badge::Badge> Card = badge::InOrder(S, badge::Card, badge::FrameNone);
		Expect(Card.size() == S.Held.size() && Card.front().K == badge::Kind::HallOfFame && Card.back().K == badge::Kind::Club, "the card: everything, Hall first, the club last");
	}
	// C1: one glyph a row; C3: room badges only in the room and on the card; Kast Partner in rows only with nothing above.
	{
		badge::Holder H;
		H.WallWins = 3;
		H.ClubVisitsNow = 40;
		const badge::Set Room = badge::Earned(H, Day);
		Expect(!badge::Top(Room, badge::Network, badge::FrameNone) && !badge::Top(Room, badge::Stream, badge::FrameNone) && badge::Top(Room, badge::Room, badge::FrameNone) &&
				   badge::InOrder(Room, badge::Card, badge::FrameNone).size() == 2,
			"C3: the wall and the club only in the room and on the card");
		badge::Holder K;
		K.KastPartner = true;
		const badge::Set KastOnly = badge::Earned(K, Day);
		const badge::Badge* Only = badge::Top(KastOnly, badge::Network, badge::FrameNone);
		K.VerifiedHeld = true;
		const badge::Set KastAndMore = badge::Earned(K, Day);
		const badge::Badge* Above = badge::Top(KastAndMore, badge::Network, badge::FrameNone);
		Expect(Only && Only->K == badge::Kind::KastPartner && Above && Above->K == badge::Kind::VerifiedPro, "C3: Kast Partner in a row only when nothing above it is held");
	}
	// C4: a Regular's card is never a glyph in rows; it stays the words, and shows on the full card.
	{
		badge::Holder H;
		H.ClubVisitsNow = 7;
		const badge::Set Regular = badge::Earned(H, Day);
		Expect(Regular.Find(badge::Kind::Club) && !badge::Top(Regular, badge::Room, badge::FrameNone) && !badge::Top(Regular, badge::Network, badge::FrameNone) &&
				   badge::Top(Regular, badge::Card, badge::FrameNone) && badge::Caption(Regular, badge::Room).empty(),
			"C4: Regular: no glyph in rows, on the card");
		H.ClubVisitsNow = 18;
		const badge::Set EmberSet = badge::Earned(H, Day);
		const badge::Badge* Ember = badge::Top(EmberSet, badge::Room, badge::FrameNone);
		Expect(Ember && Ember->Tier == 2, "C4: Ember is a glyph in the room");
	}
}

// ------------------------------------------------------------------ the words

void Words()
{
	// The desk (§5.5), word for word.
	Expect(badge::DeskLine(1, true, ", grinder") == "Six in three months. That makes you a regular, grinder. New card's in the tray.", "desk: Regular");
	Expect(badge::DeskLine(2, true, "") == "Eighteen in ninety days. Ember card. Don't lose it.", "desk: Ember");
	Expect(badge::DeskLine(3, true, ", grinder") == "Thirty-six. There's maybe six people in this room carrying that card, grinder.", "desk: Crest");
	Expect(badge::DeskLine(2, false, ", grinder") == "There's my regular, grinder." && badge::DeskLine(0, true, "").empty(), "desk: kept; nothing for no card");
	// Honorifics and the intro (§5.1, §5.5).
	{
		badge::Holder H;
		H.WallWins = 2;
		H.ClubVisitsNow = 18;
		const badge::Set Mei = badge::Earned(H, 900);
		Expect(badge::Honorific(Mei, badge::Room) == "two-time Embercrest champion", "honorific: two-time Embercrest champion");
		Expect(badge::Introduce(Mei, badge::Room, "Mei") == "two-time Embercrest champion, Mei", "intro: set off by a comma");
		Expect(badge::Caption(Mei, badge::Room) == "Twice on the wall \xC2\xB7 Ember member", "caption: twice on the wall, Ember member");
		badge::Holder P;
		P.TeamRiverLine = true;
		Expect(badge::Introduce(badge::Earned(P, 900), badge::Room, "Kenji") == "Team RiverLine's Kenji", "intro: a possessive goes on the name");
		badge::Holder E;
		E.Sponsor = "embercrest";
		Expect(badge::Introduce(badge::Earned(E, 900), badge::Room, "Mei") == "Team Embercrest's own Mei", "intro: Team Embercrest's own");
		badge::Holder R;
		std::vector<world::Award> A = Trophies(900);
		A.erase(A.begin(), A.begin() + 2);
		R.Awards = &A;
		Expect(badge::Honorific(badge::Earned(R, 900), badge::Room) == "three-time ring winner", "honorific: three-time ring winner");
		badge::Holder V;
		V.Sponsor = "voltrush";
		Expect(badge::Introduce(badge::Earned(V, 900), badge::Room, "Dana") == "Dana", "intro: a brand that isn't the room's says nothing aloud");
		// A reigning Player of the Year is read in a room intro (§5.5) though the room's seats never show it (§5.4): after the
		// trophies, before the wall; the live title first. Off the rows the year after, off the intro too.
		badge::Holder Y;
		Y.Honors = {Titled("Live Player of the Year", 2027), Titled("Player of the Year", 2027)};
		Y.WallWins = 2;
		Y.ClubVisitsNow = 20;
		const badge::Set Reign = badge::Earned(Y, world::DayOn(2028, 3, 1));
		const badge::Badge* Seat = badge::Top(Reign, badge::Room, badge::FrameNone);
		Check(badge::Introduce(Reign, badge::Room, "Mrs. Park") == "last year's Live Player of the Year, Mrs. Park" && Seat && Seat->K == badge::Kind::OnTheWall &&
				  badge::Caption(Reign, badge::Room) == "Twice on the wall \xC2\xB7 Ember member",
			"intro: a reigning Live Player of the Year is read aloud, not shown on the seat: " + badge::Introduce(Reign, badge::Room, "Mrs. Park"));
		Y.Honors.pop_back();
		Y.Honors[0].Title = "Player of the Year";
		Check(badge::Honorific(badge::Earned(Y, world::DayOn(2028, 3, 1)), badge::Room) == "last year's Player of the Year", "honorific: last year's Player of the Year");
		Check(badge::Honorific(badge::Earned(Y, world::DayOn(2029, 3, 1)), badge::Room) == "two-time Embercrest champion", "honorific: a title two years old gives way to the wall");
		badge::Holder RingAndPoy = Y;
		std::vector<world::Award> One = Trophies(900);
		One.erase(One.begin(), One.begin() + 4);
		RingAndPoy.Awards = &One;
		Check(badge::Honorific(badge::Earned(RingAndPoy, world::DayOn(2028, 3, 1)), badge::Room) == "ring winner", "honorific: the trophies come before a Player of the Year");
	}
	// The hover lines read the holder's numbers: the paid record (freerolls aside), the wall's latest, the trophy's.
	{
		const world::Tracker T = Page(1234, 50, 0.2);
		badge::Holder H;
		H.Stats = &T;
		badge::Badge V;
		V.K = badge::Kind::VerifiedPro;
		const std::string Line = badge::HowEarned(V, H);
		Check(Line == "Plays for a living, and the numbers hold: 1,234 tournaments, +20% ROI, 246 buy-ins up. Checked on the 1st of every month.", "hover: Verified Pro: " + Line);
		Check(badge::HowEarned(V, badge::Holder()) == "Plays for a living, and the numbers hold. Checked on the 1st of every month.", "hover: Verified with no record");
		badge::Holder W;
		W.WallWins = 3;
		W.WallLast = world::DayOn(2026, 11, 2);
		W.WallLastEvent = "Sunday";
		const badge::Set WallSet = badge::Earned(W, W.WallLast + 1);
		const badge::Badge* Wall = WallSet.Find(badge::Kind::OnTheWall);
		Check(Wall && badge::HowEarned(*Wall, W) == "On the Embercrest's wall. Latest: Sunday, Nov 2." && badge::Label(*Wall) == "ON THE WALL \xC3\x97" "3", "hover: the wall");
		const int Day = world::DayOn(2028, 3, 1);
		const std::vector<world::Award> A = Trophies(Day);
		badge::Holder Tr;
		Tr.Awards = &A;
		const badge::Set RingSet = badge::Earned(Tr, Day);
		const badge::Badge* Rg = RingSet.Find(badge::Kind::Ring);
		const std::string RingLine = Rg ? badge::HowEarned(*Rg, Tr) : std::string();
		Check(RingLine.rfind("3\xC3\x97 ring winner. Latest: $1,500 Bounty, Grand Circuit Montreal 2027, ", 0) == 0, "hover: rings: " + RingLine);
		badge::Badge Club;
		Club.K = badge::Kind::Club;
		Club.Tier = 1;
		Club.Count = 6;
		Check(badge::HowEarned(Club, badge::Holder()) == "6 events at the Embercrest in the last 90 days. Ember is 18. Crest is 36.", "hover: the club");
	}
	// The copy lint over every sentence (§7.6), and the label checks over every label, title, caption word and progress line.
	{
		const std::map<std::string, std::string> Extra = {{"Year", "2027"}, {"N", "3"}, {"Series", "Grand Circuit Montreal 2027"}, {"Date", "Nov 2, 2027"},
			{"Points", "4,812"}, {"Top", "200"}, {"Next", "2028"}, {"Title", "Main Event bracelet"}, {"Events", "2,814"}, {"Roi", "+23%"}, {"BuyIns", "410"},
			{"Addr", ", grinder"}, {"Short", "Nightly"}, {"Event", "$1,500 Bounty"}};
		std::vector<std::pair<std::string, std::string>> Talk;
		std::vector<std::pair<std::string, std::string>> Long;
		for (const auto& Kv : badge::Inventory())
		{
			(Kv.first.rfind("hover.", 0) == 0 || Kv.first.rfind("honor.", 0) == 0 || Kv.first.rfind("next.", 0) == 0 ? Long : Talk).push_back(Kv);
		}
		Expect(Talk.size() >= 40 && Long.size() >= 40, "copy: the inventory is all there");
		Failures += copylint::Run("badges", Talk, copylint::TableTalk, Extra);
		Failures += copylint::Run("badges", Long, copylint::Pa, Extra);
		const std::map<std::string, std::string> LabelVars = {{"N", "3"}, {"Yy", "27"}, {"BRAND", "VOLT RUSH"}, {"Year", "2027"}, {"TimesCap", "Three-time"},
			{"Brand", "Volt Rush"}, {"Tier", "Ember"}, {"Have", "612"}, {"Need", "1,000"}, {"Roi", "+12%"}, {"BuyIns", "96"}, {"Quiet", "12"}, {"Hours", "18"},
			{"Days", "9"}, {"Rank", "41"}, {"Date", "Nov 2"}, {"Words", "three"}, {"Times", "two-time"}};
		int Bad = 0;
		for (const auto& Kv : badge::Labels())
		{
			const std::string Filled = copylint::Fill(Kv.second, LabelVars);
			const std::string Low = copylint::Lower(Filled);
			const bool Ok = !Filled.empty() && Filled.find('{') == std::string::npos && Filled.find("  ") == std::string::npos && copylint::Characters(Filled) <= 64 &&
				Low.find("todo") == std::string::npos && Low.find("tbd") == std::string::npos && Low.find("placeholder") == std::string::npos && Low.find("lorem") == std::string::npos;
			if (!Ok)
			{
				++Bad;
				std::printf("LABEL (badges) %s: \"%s\"\n", Kv.first.c_str(), Filled.c_str());
			}
		}
		Expect(Bad == 0 && badge::Labels().size() >= 50, "labels: every label, title, caption word and progress line");
	}
	// Chat: one reaction per earning, the same line for the same salt, from the inventory.
	{
		badge::Badge V;
		V.K = badge::Kind::VerifiedPro;
		std::set<std::string> Lines;
		for (uint32_t Salt = 0; Salt < 9; ++Salt)
		{
			Lines.insert(badge::ChatReaction(V, Salt));
		}
		Expect(Lines.size() == 3 && badge::ChatReaction(V, 4) == badge::ChatReaction(V, 4) && Lines.count("the hexagon is real!") == 1, "chat: three lines a badge, seeded");
		int Every = 0;
		for (int K = 0; K < static_cast<int>(badge::Kind::Count); ++K)
		{
			badge::Badge B;
			B.K = static_cast<badge::Kind>(K);
			B.Brand = "voltrush";
			B.Year = 2027;
			Every += badge::ChatReaction(B, 1).empty() || badge::ChatReaction(B, 1).find('{') != std::string::npos ? 0 : 1;
		}
		Expect(Every == static_cast<int>(badge::Kind::Count), "chat: every badge has its lines");
		// Chat can't know more than the badge says: no he or she (the player is either), no card it didn't print (a Crest is
		// no "regular"), no "another" on a first win.
		int Wrong = 0;
		for (int K = 0; K < static_cast<int>(badge::Kind::Count); ++K)
		{
			for (int Tier = 1; Tier <= 3; ++Tier)
			{
				for (uint32_t Salt = 0; Salt < 3; ++Salt)
				{
					badge::Badge B;
					B.K = static_cast<badge::Kind>(K);
					B.Tier = Tier;
					B.Count = 1;
					B.Year = 2027;
					B.Brand = "voltrush";
					const std::string Said = " " + copylint::Lower(badge::ChatReaction(B, Salt)) + " ";
					const bool Bad = Said.find(" he ") != std::string::npos || Said.find(" she ") != std::string::npos || Said.find(" his ") != std::string::npos ||
						Said.find(" her ") != std::string::npos || (B.K == badge::Kind::Club && Tier >= 2 && Said.find("regular") != std::string::npos) ||
						(B.K == badge::Kind::OnTheWall && Said.find("another") != std::string::npos);
					if (Bad)
					{
						++Wrong;
						std::printf("  chat says too much: \"%s\"\n", Said.c_str());
					}
				}
			}
		}
		Expect(Wrong == 0, "chat: the reactions fit any player, any card and a first win");
	}
	// Every label and title, for every kind and tier, is something (no badge goes unnamed).
	{
		int Named = 0;
		for (const badge::Badge& B : ui::badgeart::AtlasBadges())
		{
			Named += !badge::Label(B).empty() && !badge::Title(B).empty() && !badge::HowEarned(B, badge::Holder()).empty() ? 1 : 0;
		}
		Expect(Named == static_cast<int>(ui::badgeart::AtlasBadges().size()), "words: every atlas variant has a label, a title and a hover line");
	}
}

// ------------------------------------------------------------------ the packed form and the save

void PackAndSave()
{
	const int Day = world::DayOn(2027, 5, 10);
	const std::vector<world::Award> A = Trophies(Day);
	const world::Tracker T = Page(1200, 0, 0.3);
	badge::Set S = badge::Earned(Everything(Day, A, T), Day);
	S.Held[6].Brand = "odd;brand.with%stuff and\ttabs";
	const std::string P = badge::Pack(S);
	const badge::Set Back = badge::Unpack(P);
	bool Same = Back.Held.size() == S.Held.size();
	for (size_t K = 0; Same && K < S.Held.size(); ++K)
	{
		const badge::Badge& X = S.Held[K];
		const badge::Badge& Y = Back.Held[K];
		Same = X.K == Y.K && X.Tier == Y.Tier && X.Count == Y.Count && X.Year == Y.Year && X.Main == Y.Main && X.Color == Y.Color && X.Brand == Y.Brand;
	}
	Expect(Same, "pack: a full set round-trips, brand escapes and all");
	bool Safe = true;
	for (const char C : P)
	{
		Safe = Safe && ((C >= '0' && C <= '9') || (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || C == '.' || C == ';' || C == '%' || C == '_' || C == '-');
	}
	Expect(Safe && P.find('\t') == std::string::npos, "pack: line-safe characters only");
	Expect(badge::Pack(badge::Set()).empty() && badge::Unpack("").Held.empty(), "pack: nothing is empty");
	Expect(badge::Unpack("99.1.2;garbage;11.2.18").Held.size() == 1 && badge::Unpack("11.2.18").Held[0].Tier == 2, "unpack: skips what it can't read");
	badge::Badge Wall;
	Wall.K = badge::Kind::OnTheWall;
	Wall.Count = 2;
	badge::Set One;
	One.Held.push_back(Wall);
	Expect(badge::Pack(One) == "10.0.2", "pack: trailing defaults left off");
	// badge\treview: none until a review has run; then it round-trips.
	{
		life::State L;
		std::string Out;
		badge::WriteLines(L, Out);
		Expect(Out.empty(), "save: no review line before a review");
		L.BadgeMonth = 2027 * 12 + 4;
		L.VerifiedHeld = true;
		badge::WriteLines(L, Out);
		Expect(Out == "badge\treview\t24328\t1\n", "save: badge\\treview\\t<month>\\t<verified>");
		life::State Read;
		Expect(badge::ReadLine(Tabs(Out.substr(0, Out.size() - 1)), Read) && Read.BadgeMonth == L.BadgeMonth && Read.VerifiedHeld, "save: the review line reads back");
		Expect(badge::ReadLine({"badge", "later", "x"}, Read) && !badge::ReadLine({"stake", "deal"}, Read), "save: a later badge line is skipped, others aren't ours");
	}
	// Through the whole save: a SaveData with the line writes back byte for byte.
	{
		SaveData D;
		D.Life.BadgeMonth = 24330;
		D.Life.VerifiedHeld = false;
		const std::string Text = D.Serialize();
		SaveData Back2;
		Expect(Text.find("badge\treview\t24330\t0\n") != std::string::npos && SaveData::Parse(Text, Back2) && Back2.Life.BadgeMonth == 24330 && Back2.Serialize() == Text,
			"save: the review survives a SaveData round trip");
	}
}

// ------------------------------------------------------------------ farming: the club counts nights played

void Farming()
{
	life::State L;
	Chips Bankroll = 10000000;
	// The first Monday after the story starts, and the three weeks after it.
	int Monday = net::NightOneDay;
	while (net::Weekday(Monday) != 0)
	{
		++Monday;
	}
	auto Play = [&](const live::Occurrence& O, bool Finish) {
		const std::string Why = live::Register(Bankroll, L, O, O.Start - live::TravelMinutes, 80, {});
		Check(Why.empty(), "farming: registers for " + O.Id + " (" + Why + ")");
		if (Finish)
		{
			Expect(live::Settle(Bankroll, L, O.Id, 40, 80, 0, O.Start + 300.0), "farming: settles");
		}
		else
		{
			Expect(live::Refund(Bankroll, L, O.Id, O.Start - 30.0, "called off"), "farming: refunds");
		}
	};
	auto Nightly = [](int Day) {
		for (const live::Occurrence& O : live::Occurrences(Day))
		{
			if (std::string(O.T->Key) == "nightly")
			{
				return O;
			}
		}
		return live::Occurrence();
	};
	// Six Nightlies in three weeks: Regular.
	const int Days[6] = {Monday, Monday + 2, Monday + 7, Monday + 9, Monday + 14, Monday + 16};
	for (int K = 0; K < 5; ++K)
	{
		Play(Nightly(Days[K]), true);
	}
	Expect(badge::HeroClubTier(L, Days[4]) == 0, "farming: five nights, no card");
	// A refunded entry is never a visit.
	Play(Nightly(Monday + 15), false);
	Expect(badge::VisitsIn(badge::HeroRoomDays(L), Monday + 15) == 5 && badge::HeroClubTier(L, Monday + 15) == 0, "farming: a refunded entry adds no visit");
	Play(Nightly(Days[5]), true);
	Expect(badge::HeroClubTier(L, Days[5]) == 1, "farming: six Nightlies in three weeks give Regular");
	// A second event on the same day is the same visit.
	for (const live::Occurrence& O : live::Occurrences(Monday + 17))
	{
		Play(O, true);
	}
	Expect(live::Occurrences(Monday + 17).size() >= 2 && badge::VisitsIn(badge::HeroRoomDays(L), Monday + 17) == 7, "farming: two events in a day are one visit");
	// The window moves on: 91 days after the first night it no longer counts.
	Expect(badge::VisitsIn(badge::HeroRoomDays(L), Days[0] + 90) == 6 && badge::VisitsIn(badge::HeroRoomDays(L), Monday + 17 + 90) == 0,
		"farming: the 90-day window lets nights go");
	// Freerolls make neither Verified's count nor its ROI (S9).
	{
		const world::Tracker Free = Page(999, 40, 0.10);
		badge::Holder H;
		H.Stats = &Free;
		H.Professional = true;
		H.DaysSincePlayed = 0;
		badge::Badge V;
		V.K = badge::Kind::VerifiedPro;
		Expect(!badge::VerifiedNow(H) && Free.Events == 1039 && badge::HowEarned(V, H).find("999 tournaments, +10% ROI") != std::string::npos,
			"farming: freerolls move neither Verified's count nor its ROI");
	}
}

// ------------------------------------------------------------------ the world's people

std::string Packs(const world::World& W, int Day)
{
	std::string Out;
	for (const world::Npc& N : W.People())
	{
		const std::string P = badge::Pack(badge::Earned(badge::Of(W, N.Id, Day), Day));
		if (!P.empty())
		{
			Out += std::to_string(N.Id) + "=" + P + "\n";
		}
	}
	return Out;
}

void WorldPeople()
{
	world::World W;
	const auto Made = std::chrono::steady_clock::now();
	W.Create(1234u, NightOne);
	W.Simulate(45);
	std::printf("  world 1234, 45 days: %.2f s\n", Seconds(Made));
	const int Day = net::DayOf(W.Clock());
	// The index: the rules' answer, built once a person, a day and a revision; the whole roster in budget (§10).
	{
		badge::Index Ix;
		const auto Start = std::chrono::steady_clock::now();
		bool Same = true;
		for (const world::Npc& N : W.People())
		{
			const badge::Set& A = Ix.Of(W, N.Id, Day);
			Same = Same && badge::Pack(A) == badge::Pack(badge::Earned(badge::Of(W, N.Id, Day), Day));
		}
		const double Ms = Seconds(Start) * 1000.0;
		std::printf("  badge index: %zu people in %.2f ms (with the checks)\n", W.People().size(), Ms);
		const auto Build = std::chrono::steady_clock::now();
		badge::Index Fresh;
		for (const world::Npc& N : W.People())
		{
			(void)Fresh.Of(W, N.Id, Day);
		}
		const double BuildMs = Seconds(Build) * 1000.0;
		std::printf("  badge index: the whole roster built in %.2f ms (budget 5 ms)\n", BuildMs);
		Expect(Same, "index: every set is the rules'");
		Expect(Ix.BuiltCount() == static_cast<int>(W.People().size()), "index: one build a person");
		(void)Ix.Of(W, 0, Day);
		Expect(Ix.BuiltCount() == static_cast<int>(W.People().size()), "index: asking again builds nothing");
		(void)Ix.Of(W, 0, Day + 1);
		Expect(Ix.BuiltCount() == 1, "index: a new day starts over (lazily)");
		Check(BuildMs < 25.0, "index: the roster builds well inside its budget (" + Fixed(BuildMs, 2) + " ms)");
		Expect(&Ix.Of(W, -1, Day) == &Ix.Of(W, 999999, Day) && Ix.Of(W, -1, Day).Held.empty(), "index: nobody, nothing");
	}
	// Privacy (C6, N3): a backer, makeup and a bankroll change nothing.
	{
		world::World Copy = W;
		const std::string Before = Packs(Copy, Day);
		for (const world::Npc& N : Copy.People())
		{
			world::Npc& M = const_cast<world::Npc&>(N);
			M.Backer = M.Backer == -1 ? -2 : -1;
			M.BackerShare = 0.5f;
			M.Makeup += 123456;
			M.Bankroll = M.Bankroll * 3 + 777;
		}
		Expect(Before == Packs(Copy, Day) && !Before.empty(), "privacy: Backer, Makeup and Bankroll leave every badge as it was");
	}
	// The room's snapshot: the field's packs (only people who hold any), and the month's leaders.
	{
		std::vector<int> Field;
		for (const world::Npc& N : W.People())
		{
			if (N.Room.Wins > 0 || N.Room.VisitsIn(Day) >= badge::ClubEmber || !N.Awards.empty())
			{
				Field.push_back(N.Id);
			}
			if (Field.size() >= 113)
			{
				break;
			}
		}
		Field.push_back(0);
		const int Builds = badge::Shared().BuiltCount();
		const auto Timed = std::chrono::steady_clock::now();
		const auto Notes = badge::FieldBadges(W, Field, Day);
		const double FieldMs = Seconds(Timed) * 1000.0;
		std::printf("  field: %zu people's badges in %.2f ms (the desk's worker, off the game thread)\n", Field.size(), FieldMs);
		Check(FieldMs < 50.0, "field: the snapshot's badges are cheap (" + Fixed(FieldMs, 2) + " ms)");
		Expect(badge::Shared().BuiltCount() == Builds, "field: FieldBadges never touches the game thread's index (the desk's worker runs it)");
		bool Right = !Notes.empty();
		int Plates = 0;
		for (const auto& Kv : Notes)
		{
			const int Id = W.Find(Kv.first);
			const badge::Set Held = badge::Unpack(Kv.second);
			const badge::Badge* Poy = Held.Find(badge::Kind::PlayerOfYear);
			const badge::Badge* LivePoy = Held.Find(badge::Kind::LivePlayerOfYear);
			Right = Right && Id >= 0 && !Held.Held.empty() && (!Poy || Poy->Tier >= 1) && (!LivePoy || LivePoy->Tier >= 1) && !Held.Find(badge::Kind::KastPartner);
			const badge::Set Full = badge::Earned(badge::Of(W, Id, Day), Day);
			const badge::Badge* A = badge::Top(Held, badge::Room, badge::FrameNone);
			const badge::Badge* B = badge::Top(Full, badge::Room, badge::FrameNone);
			Right = Right && ((!A && !B) || (A && B && A->K == B->K && A->Tier == B->Tier && A->Count == B->Count));
			Plates += A ? 1 : 0;
		}
		Expect(Right, "field: names to packs, the room's badges only, the same top glyph as the full set");
		std::printf("  field: %zu of %zu with badges, %d with a plate glyph\n", Notes.size(), Field.size(), Plates);
		const std::vector<life::RoomLeader> Leaders = badge::FieldLeaders(W, Day);
		bool Ranked = !Leaders.empty() && Leaders.size() <= 10;
		for (size_t K = 0; K < Leaders.size(); ++K)
		{
			Ranked = Ranked && Leaders[K].Rank == static_cast<int>(K) + 1 && Leaders[K].Points > 0.0 && !Leaders[K].Name.empty() &&
				(K == 0 || Leaders[K - 1].Points >= Leaders[K].Points);
		}
		Expect(Ranked, "leaders: up to ten, ranked by the month's points");
		// A reigning Live Player of the Year in the field: the snapshot keeps the title for the floor's intro (§5.5), the seat
		// still shows the room's own glyph (§5.4); a title two years old stays home.
		std::vector<int> Plain; // no trophies, so the title is the one to read
		for (const int Id : Field)
		{
			const world::Npc* N = W.Get(Id);
			if (N && N->Awards.empty() && W.HallClass(Id) == 0 && Plain.size() < 2)
			{
				Plain.push_back(Id);
			}
		}
		Check(Plain.size() == 2, "field: two people without trophies to crown");
		for (size_t K = 0; Plain.size() < 2 && K < Field.size(); ++K)
		{
			Plain.push_back(Field[K]);
		}
		std::string Text;
		W.Write(Text);
		const int Year = world::YearOf(Day);
		Text += "world\thonor\t" + std::to_string(Year - 1) + "\tLive Player of the Year\t\t" + std::to_string(Plain[0]) + "\t" + W.Get(Plain[0])->Name + "\t0\t4812\t0\n";
		Text += "world\thonor\t" + std::to_string(Year - 2) + "\tPlayer of the Year\t\t" + std::to_string(Plain[1]) + "\t" + W.Get(Plain[1])->Name + "\t0\t9000\t0\n";
		world::World Crowned;
		LoadWorld(Crowned, Text);
		const auto Two = badge::FieldBadges(Crowned, Plain, Day);
		const badge::Set Reigning = Two.size() == 2 ? badge::Unpack(Two[0].second) : badge::Set();
		const badge::Set Past = Two.size() == 2 ? badge::Unpack(Two[1].second) : badge::Set();
		const badge::Badge* Kept = Reigning.Find(badge::Kind::LivePlayerOfYear);
		const badge::Badge* SeatTop = badge::Top(Reigning, badge::Room, badge::FrameNone);
		Check(Kept && Kept->Tier == 1 && (!SeatTop || SeatTop->K != badge::Kind::LivePlayerOfYear) && badge::Honorific(Reigning, badge::Room).find("Live Player of the Year") != std::string::npos &&
				  !Past.Find(badge::Kind::PlayerOfYear),
			"field: a reigning Live Player of the Year rides in the snapshot for the intro (" + (Two.empty() ? std::string() : Two[0].second) + ")");
	}
	// The Hall's rule at its edges: 39/40, 9/10 years on the scene, and the stakes (the world's HallEligible, the badges' bar).
	{
		world::World Copy = W;
		const int Year = world::YearOf(Day) + 1;
		int Id = -1;
		for (const world::Npc& N : Copy.People())
		{
			if (!N.Faded && Copy.HallClass(N.Id) == 0)
			{
				Id = N.Id;
				break;
			}
		}
		world::Npc& N = const_cast<world::Npc&>(*Copy.Get(Id));
		N.Born = Year - 40;
		N.Joined = world::DayOn(Year - 10, 1, 1);
		N.Live = world::LiveLevel::Circuit;
		const bool Ok = Copy.HallEligible(Id, Year);
		N.Born = Year - 39;
		const bool Young = Copy.HallEligible(Id, Year);
		N.Born = Year - 40;
		N.Joined = world::DayOn(Year - 9, 1, 1);
		const bool New = Copy.HallEligible(Id, Year);
		N.Joined = world::DayOn(Year - 10, 1, 1);
		N.Live = world::LiveLevel::Local;
		N.Tier = 1;
		N.Stats.ByStake[static_cast<size_t>(world::TrackStake::High)].Events = 0;
		N.Reps[static_cast<size_t>(world::Rep::HighStakes)] = 0.0f;
		const bool Low = Copy.HallEligible(Id, Year);
		Expect(Ok && !Young && !New && !Low, "Hall: 40 and 10 years and high stakes, not 39, not 9 years, not without the stakes");
		Expect(std::fabs(badge::HallScore(N, Copy, Year) - Copy.HallScore(Id, Year)) < 1e-12, "Hall: the badges' score is the world's");
	}
}

// ------------------------------------------------------------------ migration from world v1

void Migration()
{
	const std::string Text = ReadText(Fixture("world_v1.txt"));
	Check(Text.rfind("world\thead\t1\t", 0) == 0, "migration: the version 1 world is there (" + Fixture("world_v1.txt") + ")");
	if (Text.empty())
	{
		return;
	}
	world::World W;
	LoadWorld(W, Text);
	const int Day = net::DayOf(W.Clock());
	int Wall = 0;
	int Club[4] = {0, 0, 0, 0};
	int Verified = 0;
	for (const world::Npc& N : W.People())
	{
		const badge::Set S = badge::Earned(badge::Of(W, N.Id, Day), Day);
		Wall += S.Find(badge::Kind::OnTheWall) ? 1 : 0;
		const badge::Badge* C = S.Find(badge::Kind::Club);
		++Club[C ? C->Tier : 0];
		Verified += S.Find(badge::Kind::VerifiedPro) ? 1 : 0;
	}
	Check(Wall == 0, "migration: nobody is on the wall without a win (" + std::to_string(Wall) + ")");
	Check(Club[1] + Club[2] + Club[3] >= 20 && Club[3] <= 15, "migration: the room's months are seeded (" + std::to_string(Club[1]) + " Regular, " + std::to_string(Club[2]) +
			  " Ember, " + std::to_string(Club[3]) + " Crest)");
	Check(Verified >= 30 && Verified <= 220, "migration: the first review ran (" + std::to_string(Verified) + " Verified)");
	// Written as version 2 and read again, every badge is the same.
	std::string Again;
	W.Write(Again);
	world::World Back;
	LoadWorld(Back, Again);
	Expect(Packs(W, Day) == Packs(Back, Day), "migration: written as version 2 and read back, the badges are the same");
	std::printf("  world v1: %d Regular, %d Ember, %d Crest, %d Verified\n", Club[1], Club[2], Club[3], Verified);
}

// ------------------------------------------------------------------ determinism and distribution

struct Census
{
	int Team = 0;
	int Verified = 0;
	int Regular = 0;
	int Ember = 0;
	int Crest = 0;
	int Card = 0;
	int Wall = 0;
	int Sponsored = 0;
	int Bracelet = 0;
	int Ring = 0;
};

Census Count(const world::World& W, int Day)
{
	Census C;
	badge::Index Ix;
	for (const world::Npc& N : W.People())
	{
		const badge::Set& S = Ix.Of(W, N.Id, Day);
		C.Team += S.Find(badge::Kind::TeamRiverLine) ? 1 : 0;
		C.Verified += S.Find(badge::Kind::VerifiedPro) ? 1 : 0;
		C.Card += S.Find(badge::Kind::CircuitCard) ? 1 : 0;
		C.Wall += S.Find(badge::Kind::OnTheWall) ? 1 : 0;
		C.Sponsored += S.Find(badge::Kind::Sponsored) ? 1 : 0;
		C.Bracelet += S.Find(badge::Kind::Bracelet) ? 1 : 0;
		C.Ring += S.Find(badge::Kind::Ring) ? 1 : 0;
		if (const badge::Badge* Cl = S.Find(badge::Kind::Club))
		{
			(Cl->Tier >= 3 ? C.Crest : Cl->Tier == 2 ? C.Ember : C.Regular) += 1;
		}
	}
	return C;
}

void Distribution(bool Quick)
{
	const uint32_t Seeds[3] = {7u, 1234u, 98765u};
	std::map<int, std::string> Snapshots; // seed 7's packs at days 100..400 (determinism)
	for (const uint32_t Seed : Seeds)
	{
		if (Quick && Seed != 7u)
		{
			continue;
		}
		const auto Start = std::chrono::steady_clock::now();
		world::World W;
		W.Create(Seed, NightOne);
		const int First = net::DayOf(W.Clock());
		Census Lo;
		Census Hi;
		Lo.Team = Lo.Verified = Lo.Ember = Lo.Crest = 1 << 30;
		bool Ok = true;
		int CrestSum = 0;
		int Samples = 0;
		const int Span = Quick ? 400 : 730;
		for (int Ran = 0; Ran < Span; Ran += 20)
		{
			W.Simulate(20);
			const int Day = net::DayOf(W.Clock());
			const Census C = Count(W, Day);
			Lo.Team = std::min(Lo.Team, C.Team);
			Hi.Team = std::max(Hi.Team, C.Team);
			Lo.Verified = std::min(Lo.Verified, C.Verified);
			Hi.Verified = std::max(Hi.Verified, C.Verified);
			Lo.Ember = std::min(Lo.Ember, C.Ember + C.Crest);
			Hi.Ember = std::max(Hi.Ember, C.Ember + C.Crest);
			Lo.Crest = std::min(Lo.Crest, C.Crest);
			Hi.Crest = std::max(Hi.Crest, C.Crest);
			Hi.Card = std::max(Hi.Card, C.Card);
			Hi.Wall = std::max(Hi.Wall, C.Wall);
			Hi.Sponsored = std::max(Hi.Sponsored, C.Sponsored);
			// §7.6's bands: Team RiverLine 10, Verified 30 to 220, Ember (an Ember card or better) 25 to 50, Crest 2 to 15 (a quiet
			// stretch can leave one: every sample at least 1, the average at least 2), Card at most 260. The room's bands hold once
			// the history the world seeds at its start (the 120 days before Night One) has left the 90-day window.
			const bool Settled = Ran + 20 >= badge::ClubWindowDays;
			const bool RoomOk = !Settled || (C.Crest >= 1 && C.Crest <= 15 && C.Ember + C.Crest >= 25 && C.Ember + C.Crest <= 50);
			const bool Here = RoomOk && C.Team == 10 && C.Verified >= 30 && C.Verified <= 220 && C.Card <= 260;
			CrestSum += Settled ? C.Crest : 0;
			Samples += Settled ? 1 : 0;
			if (!Here)
			{
				std::printf("  seed %u, day %d: Team %d, Verified %d, Regular %d, Ember %d, Crest %d, Card %d\n", Seed, Day - First, C.Team, C.Verified, C.Regular, C.Ember,
					C.Crest, C.Card);
			}
			Ok = Ok && Here;
			if (Seed == 7u && (Ran + 20) % 100 == 0 && Ran + 20 <= 400)
			{
				Snapshots[Ran + 20] = Packs(W, Day);
			}
		}
		// The Hall: classes of at most two a year.
		std::map<int, int> Classes;
		for (const world::Honor& H : W.Honors())
		{
			Classes[H.Year] += H.Title == world::HallOfFameTitle ? 1 : 0;
		}
		bool Small = true;
		for (const auto& Kv : Classes)
		{
			Small = Small && Kv.second <= badge::HallClass;
		}
		const double CrestMean = Samples > 0 ? static_cast<double>(CrestSum) / static_cast<double>(Samples) : 0.0;
		Check(Ok && Small && CrestMean >= 2.0, "distribution, seed " + std::to_string(Seed) + ": Team RiverLine 10, Verified 30-220, Ember 25-50, Crest 2-15 (average " + Fixed(CrestMean, 1) + "), Card <= 260, Hall <= 2 a year");
		std::printf("  seed %u, %d days (%.1f s): Team %d-%d, Verified %d-%d, Ember or better %d-%d, Crest %d-%d, Card <= %d, Wall <= %d, Sponsored <= %d\n", Seed, Span, Seconds(Start),
			Lo.Team, Hi.Team, Lo.Verified, Hi.Verified, Lo.Ember, Hi.Ember, Lo.Crest, Hi.Crest, Hi.Card, Hi.Wall, Hi.Sponsored);
	}
	// Determinism over 400 days: the same seed, the same badges for everyone, day by day.
	{
		world::World W;
		W.Create(7u, NightOne);
		bool Same = Snapshots.size() == 4;
		for (int Ran = 0; Ran < 400; Ran += 20)
		{
			W.Simulate(20);
			const auto It = Snapshots.find(Ran + 20);
			if (It != Snapshots.end())
			{
				Same = Same && It->second == Packs(W, net::DayOf(W.Clock()));
			}
		}
		Expect(Same, "determinism: 400 days, the same seed, every badge the same on days 100, 200, 300 and 400");
	}
}

// ------------------------------------------------------------------ the session's side

struct Hooks : SessionHooks
{
	int Saves = 0;
	SaveData Last;
	void Save(const SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

void SessionSide()
{
	Hooks H;
	Session S(H, "badges");
	S.CurrentScreen = Screen::Lobby;
	const double Now = S.WorldMinutes();
	const int Today = net::DayOf(Now);
	// On load: the review ran (once), the Hall knows the player's age, nothing held, nothing to celebrate.
	Expect(S.Life.BadgeMonth == world::MonthKeyOf(world::MonthStart(Today) - 1) || S.Life.BadgeMonth == world::MonthKeyOf(Today), "session: the review runs on load");
	Expect(!S.Life.VerifiedHeld && S.HeroBadges().Held.empty() && S.Unseen().empty(), "session: a fresh career holds nothing");
	Expect(S.Living().HeroBorn == world::YearOf(net::DayOf(S.Living().StartedAt())) - S.Person.Age, "session: the world knows the player's birth year (the Hall's rule)");
	// The club from the live record: the debug grant writes real nights, which the card room's save reads too.
	const std::string Granted = S.DebugBadge("grant Club 2");
	const badge::Badge* Club = S.HeroBadges().Find(badge::Kind::Club);
	Check(Club && Club->Tier == 2 && badge::HeroClubTier(S.Life, Today) == 2 && Granted.rfind("Granted: EMBER.", 0) == 0, "session: ss.Badge.Grant Club 2: " + Granted);
	// C10: celebrated once; the key is in TextsSeen, so a reload doesn't celebrate it again.
	const std::vector<badge::Badge> First = S.Unseen();
	Expect(First.size() == 1 && First[0].K == badge::Kind::Club && S.Unseen().empty(), "session: an earning is celebrated once (C10)");
	S.Save();
	const SaveData Saved = H.Last;
	Expect(std::find(Saved.TextsSeen.begin(), Saved.TextsSeen.end(), badge::SeenKey(First[0])) != Saved.TextsSeen.end(), "session: the celebration's key is saved");
	{
		Hooks H2;
		Session Again(H2, "badges", &Saved);
		const badge::Badge* Kept = Again.HeroBadges().Find(badge::Kind::Club);
		Expect(Kept && Kept->Tier == 2 && Again.Unseen().empty(), "session: after a reload, the card is kept and not celebrated again");
	}
	// C10 on the way down: the grant went straight to Ember, so Regular was never toasted; a drop to it isn't toasted either.
	{
		const std::vector<life::LiveDay> Kept = S.Life.LiveDays;
		std::vector<life::LiveDay> Seven;
		for (const life::LiveDay& D : Kept)
		{
			if (D.Embercrest > 0 && Seven.size() < 7)
			{
				Seven.push_back(D);
			}
		}
		S.Life.LiveDays = Seven;
		const badge::Badge* Down = S.HeroBadges().Find(badge::Kind::Club);
		const std::vector<badge::Badge> Dropped = S.Unseen();
		Expect(Down && Down->Tier == 1 && Dropped.empty(), "session: a card below one already celebrated is never toasted (C10)");
		S.Life.LiveDays = Kept;
		const badge::Badge* Back = S.HeroBadges().Find(badge::Kind::Club);
		Expect(Back && Back->Tier == 2 && S.Unseen().empty(), "session: back up to a card already celebrated, no toast");
	}
	// Kast Partner, the wall: the real record moves and the badges follow at once.
	S.DebugBadge("grant KastPartner");
	S.DebugBadge("grant Wall 2");
	const badge::Badge* Stream = badge::Top(S.HeroBadges(), badge::Stream, badge::FrameNone);
	Expect(Stream && Stream->K == badge::Kind::KastPartner && S.Channel.Partner, "session: Kast Partner tops the stream plate");
	Expect(badge::Honorific(S.HeroBadges(), badge::Room) == "two-time Embercrest champion", "session: the wall in the room");
	// The player's own row in the night's snapshot (the director's intro): the room's badges from the world and the save,
	// never the stream's; the same from the save's text alone (the desk's worker has no session).
	{
		const std::string Pack = badge::HeroFieldPack(S.Living(), S.Life, S.WorldMinutes());
		const badge::Set Mine = badge::Unpack(Pack);
		const badge::Badge* Club2 = Mine.Find(badge::Kind::Club);
		const badge::Badge* Wall2 = Mine.Find(badge::Kind::OnTheWall);
		Check(Club2 && Club2->Tier == 2 && Wall2 && Wall2->Count == 2 && !Mine.Find(badge::Kind::KastPartner) &&
				  badge::Introduce(Mine, badge::Room, "Jesse") == "two-time Embercrest champion, Jesse",
			"session: the player's own snapshot row (" + Pack + ")");
		S.Save();
		SaveData Text;
		Expect(SaveData::Parse(H.Last.Serialize(), Text) && badge::HeroFieldPack(S.Living(), Text.Life, S.WorldMinutes()) == Pack, "session: the snapshot row from the save's text");
	}
	const std::vector<badge::Badge> Two = S.Unseen();
	Expect(Two.size() == 2 && S.Unseen().empty(), "session: two new earnings, celebrated once each");
	// A debug grant the world's calendar would make (a card), kept as a grant; Clear takes it away.
	S.DebugBadge("grant Card 3");
	const badge::Badge* Card = S.HeroBadges().Find(badge::Kind::CircuitCard);
	Expect(Card && Card->Tier == 3, "session: ss.Badge.Grant Card 3");
	S.DebugBadge("clear");
	Expect(!S.HeroBadges().Find(badge::Kind::CircuitCard) && S.HeroBadges().Find(badge::Kind::Club), "session: Clear takes back the debug grants only");
	// Verified: the review is the verdict until the next 1st, 6 AM; a reload never re-runs it.
	S.DebugBadge("grant Verified");
	Expect(S.HeroBadges().Find(badge::Kind::VerifiedPro) != nullptr, "session: ss.Badge.Grant Verified");
	S.BadgeStep(Now, S.WorldMinutes());
	Expect(S.Life.VerifiedHeld, "session: no review mid-month");
	const int Month = S.Life.BadgeMonth;
	S.WorldSkip(world::MonthStart(Today) + 31 - Today + 2);
	S.BadgeStep(S.WorldMinutes() - 10.0, S.WorldMinutes());
	Expect(S.Life.BadgeMonth > Month && !S.Life.VerifiedHeld && !S.HeroBadges().Find(badge::Kind::VerifiedPro), "session: the 1st's review takes Verified back from a record that doesn't hold it");
	// The calendar's step over a 30-day skip (§10: the three steps together within 3 ms): one review at most, one rebuild.
	{
		S.Life.BadgeMonth = -1;
		const auto Timed = std::chrono::steady_clock::now();
		S.BadgeStep(S.WorldMinutes() - 30.0 * 1440.0, S.WorldMinutes());
		const double StepMs = Seconds(Timed) * 1000.0;
		std::printf("  BadgeStep over 30 days: %.3f ms\n", StepMs);
		Check(S.Life.BadgeMonth >= 0 && StepMs < 3.0, "session: BadgeStep over a 30-day skip runs inside its budget (" + Fixed(StepMs, 3) + " ms)");
	}
	// Career's progress: the next club card, Verified's first test (a shift resets the 30 days), the channel.
	S.Life.LastShiftAt = S.WorldMinutes() - 3.0 * 1440.0;
	const std::vector<badge::Progress> P = S.BadgeProgress(3);
	bool Club3 = false;
	bool Verified = false;
	for (const badge::Progress& Pr : P)
	{
		Club3 = Club3 || (Pr.K == badge::Kind::Club && Pr.Line.rfind("Crest \xC2\xB7 ", 0) == 0 && Pr.Need == 36.0);
		Verified = Verified || (Pr.K == badge::Kind::VerifiedPro && Pr.Line.find("of 1,000 tournaments") != std::string::npos && Pr.Detail.rfind("Last ShiftLink shift: ", 0) == 0);
	}
	Check(P.size() >= 2 && P.size() <= 3 && Club3 && Verified, "session: Career's progress cards (" + (P.empty() ? std::string() : P[0].Line + " | " + P[0].Detail) + ")");
	// The debug command's other verbs answer.
	Expect(S.DebugBadge("describe me").find("REGULAR") == std::string::npos && S.DebugBadge("describe me").find("EMBER") != std::string::npos, "session: Describe me");
	Expect(S.DebugBadge("atlas").find("cells") != std::string::npos && S.DebugBadge("next").find("Crest") != std::string::npos, "session: Atlas and Next");
	Expect(S.DebugBadge("grant Nonsense").rfind("ss.Badge.Grant: no badge", 0) == 0 && !S.DebugBadge("").empty(), "session: an unknown grant says what there is");
	// The network's seat glyph is the player's Network top (Session.cpp fills SeatVis::Top from HeroBadges).
	const badge::Badge* Net = badge::Top(S.HeroBadges(), badge::Network, badge::FrameNone);
	Expect(Net && Net->K == badge::Kind::KastPartner, "session: the player's network glyph");
}

/** A career that has never played live: a debug wall survives the save; Kast's progress card reads Kast's own window. */
void FreshCareer()
{
	{
		Hooks H;
		Session S(H, "fresh-wall");
		S.DebugBadge("grant Wall 2");
		S.Save();
		SaveData Saved;
		Expect(SaveData::Parse(H.Last.Serialize(), Saved), "session: the fresh career's save parses");
		Hooks H2;
		Session Again(H2, "fresh-wall", &Saved);
		const badge::Badge* Wall = Again.HeroBadges().Find(badge::Kind::OnTheWall);
		Expect(Wall && Wall->Count == 2 && Again.Life.LiveWins == 2, "session: ss.Badge.Grant Wall on a fresh career survives a reload");
	}
	{
		Hooks H;
		Session S(H, "fresh-kast");
		const double Now = S.WorldMinutes();
		const int Today = net::DayOf(Now);
		// One stream in the log, three days live: the log keeps streams and the days live are their own list (Channel::Window30).
		S.Channel.Streams = 4;
		S.Channel.Log.clear();
		kast::StreamLog Night;
		Night.Start = Now - 2.0 * 1440.0;
		Night.Minutes = 150.0;
		Night.Avg = 30;
		S.Channel.Log.push_back(Night);
		S.Channel.DaysLive = {Today - 9, Today - 5, Today - 2};
		double Minutes = 0.0;
		int Days = 0;
		double Avg = 0.0;
		S.Channel.Window30(Now, Minutes, Days, Avg);
		const badge::Progress* Kast = nullptr;
		const std::vector<badge::Progress> P = S.BadgeProgress(4);
		for (const badge::Progress& Pr : P)
		{
			Kast = Pr.K == badge::Kind::KastPartner ? &Pr : Kast;
		}
		Check(Kast && Days == 3 && Kast->Line == "Kast Partner \xC2\xB7 30 of 75 average viewers" && Kast->Detail == "2 of 25 hours \xC2\xB7 3 of 12 days \xC2\xB7 last 30 days",
			"session: Kast Partner's card reads Kast's own 30 days (" + (Kast ? Kast->Line + " | " + Kast->Detail : std::string("none")) + ")");
	}
}

/** A save from before the segment (the team_riverline fixture): its Kast deal migrates into the book, and the badge follows. */
void OldSaves()
{
	for (const char* Name : {"pre_segment_fresh.ssave", "pre_segment_team_riverline.ssave", "pre_segment_mid_nightly.ssave"})
	{
		const std::string Text = ReadText(Fixture(Name));
		SaveData D;
		if (Text.empty() || !SaveData::Parse(Text, D))
		{
			Check(false, std::string("old saves: ") + Name + " loads");
			continue;
		}
		Expect(D.Life.BadgeMonth == -1, "old saves: no review line before the segment");
		Hooks H;
		Session S(H, "old-save", &D);
		Expect(S.Life.BadgeMonth >= 0, "old saves: the review runs at once on load (§7.5 step 4)");
		const bool Team = std::string(Name) == "pre_segment_team_riverline.ssave";
		if (Team)
		{
			const badge::Badge* B = badge::Top(S.HeroBadges(), badge::Stream, badge::FrameNone);
			// §9.1: the migrated deal is still running, so the check below is never the empty case.
			Check(S.TeamRiverLine(), std::string("old saves: the fixture's Team RiverLine deal is running after the migration (") + Name + ")");
			Check(S.TeamRiverLine() == (S.HeroBadges().Find(badge::Kind::TeamRiverLine) != nullptr), std::string("old saves: Team RiverLine's badge follows the deal (") + Name + ")");
			Check(!S.TeamRiverLine() || (B && B->K == badge::Kind::TeamRiverLine), "old saves: the stream plate shows Team RiverLine");
		}
		S.Save();
		SaveData Back;
		Expect(SaveData::Parse(H.Last.Serialize(), Back) && Back.Life.BadgeMonth == S.Life.BadgeMonth, "old saves: the review line is written and reads back");
	}
}
} // namespace badges_test

int main(int Argc, char** Argv)
{
	using namespace badges_test;
	const bool Quick = Argc >= 2 && std::strcmp(Argv[1], "quick") == 0;
	const auto Start = std::chrono::steady_clock::now();
	Failures += copylint::SelfCheck();
	Edges();
	Priority();
	Words();
	PackAndSave();
	Farming();
	WorldPeople();
	Migration();
	SessionSide();
	FreshCareer();
	OldSaves();
	Distribution(Quick);
	std::printf("badges tests: %.1f s\n", Seconds(Start));
	if (Failures == 0)
	{
		std::printf("badges tests: all passed\n");
	}
	else
	{
		std::printf("badges tests: %d failed\n", Failures);
	}
	return Failures == 0 ? 0 : 1;
}
