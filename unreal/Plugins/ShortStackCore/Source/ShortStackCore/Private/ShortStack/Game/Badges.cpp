#include "ShortStack/Game/Badges.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/EventArt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

// Pro badges (next_segment_spec §5). core-badges.
//
// The rules for all eleven, the slot orders and the anti-clutter rules (C1 to C10), every line of badge copy, the field's
// snapshot for the card room, the packed form the room keeps, and the index the network's screens read. Everything here
// reads public facts only (N3, C6): results, titles, rankings, sponsors, the stream's status and the room's record.

namespace ss
{
namespace badge
{
namespace badge_detail
{
constexpr const char* PoyTitle = "Player of the Year";
constexpr const char* LivePoyTitle = "Live Player of the Year";
constexpr double DayMinutes = 1440.0;

/** The copy: every template by key (Inventory and Labels read the same table, so the lint sees what ships). */
const std::map<std::string, std::string>& Copy()
{
	static const std::map<std::string, std::string> Lines = {
		// Hover lines (§5.5): one sentence or two, the tooltip's last line.
		{"hover.hall", "Hall of Fame, class of {Year}. Forty or older, ten years on the scene, and a record that held up against the best."},
		{"hover.bracelet.one", "Bracelet winner: {Event}, {Series}, {Date}."},
		{"hover.bracelet.many", "{N}\xC3\x97 bracelet winner. Latest: {Event}, {Series}, {Date}."},
		{"hover.bracelet.bare", "Bracelet winner. The trophy case is on the player card."},
		{"hover.bracelet.manybare", "{N}\xC3\x97 bracelet winner. The trophy case is on the player card."},
		{"hover.ring.one", "Ring winner: {Event}, {Series}, {Date}."},
		{"hover.ring.many", "{N}\xC3\x97 ring winner. Latest: {Event}, {Series}, {Date}."},
		{"hover.ring.bare", "Ring winner. The trophy case is on the player card."},
		{"hover.ring.manybare", "{N}\xC3\x97 ring winner. The trophy case is on the player card."},
		{"hover.poy", "Player of the Year, {Year}. Most points on RiverLine all year: {Points}."},
		{"hover.poy.bare", "Player of the Year, {Year}. Most points on RiverLine all year."},
		{"hover.livepoy", "Live Player of the Year, {Year}: {Points} points."},
		{"hover.livepoy.bare", "Live Player of the Year, {Year}. Regional events and up."},
		{"hover.teamriverline", "On Team RiverLine. The gold patch goes wherever they play."},
		{"hover.sponsored", "Sponsored by {Brand}. Wears the patch on stream and at the table."},
		{"hover.sponsored.bare", "Sponsored. Wears the patch on stream and at the table."},
		{"hover.card", "Top {Top} in circuit points in {Year}, regional events and up. Good through {Next}."},
		{"hover.card.title", "A {Title} in {Year} put them on the circuit's card. Good through {Next}."},
		{"hover.verified", "Plays for a living, and the numbers hold: {Events} tournaments, {Roi} ROI, {BuyIns} buy-ins up. Checked on the 1st of every month."},
		{"hover.verified.bare", "Plays for a living, and the numbers hold. Checked on the 1st of every month."},
		{"hover.kast", "Verified on Kast."},
		{"hover.wall", "On the Embercrest's wall. Latest: {Short}, {Date}."},
		{"hover.wall.date", "On the Embercrest's wall. Latest: {Date}."},
		{"hover.wall.bare", "On the Embercrest's wall."},
		{"hover.club.regular", "{N} events at the Embercrest in the last 90 days. Ember is 18. Crest is 36."},
		{"hover.club.ember", "{N} events at the Embercrest in the last 90 days. Crest is 36."},
		{"hover.club.crest", "{N} events at the Embercrest in the last 90 days. Crest is the top card."},
		{"hover.club.held", "{N} events at the Embercrest in the last 90 days. The card holds until the 1st."},
		// The desk (§5.5), said once per upgrade; the greeting for a member.
		{"desk.regular", "Six in three months. That makes you a regular{Addr}. New card's in the tray."},
		{"desk.ember", "Eighteen in ninety days. Ember card{Addr}. Don't lose it."},
		{"desk.crest", "Thirty-six. There's maybe six people in this room carrying that card{Addr}."},
		{"desk.kept", "There's my regular{Addr}."},
		// Kast chat, once per badge earned live (§5.6).
		{"chat.hall.0", "HALL OF FAME. on stream. i was here!"},
		{"chat.hall.1", "a legend, live, and we're watching?"},
		{"chat.hall.2", "class of {Year}, chat. show some respect!"},
		{"chat.bracelet.0", "BRACELET!!"},
		{"chat.bracelet.1", "a bracelet?? on stream??"},
		{"chat.bracelet.2", "put it on the wrist, put it on the wrist!"},
		{"chat.ring.0", "RING!! let's go!"},
		{"chat.ring.1", "a ring on stream, i'm not crying, you are."},
		{"chat.ring.2", "jewelry check. gold confirmed!"},
		{"chat.poy.0", "PLAYER OF THE YEAR!"},
		{"chat.poy.1", "poy. no debate. none!"},
		{"chat.poy.2", "best of the year and still grinding at 2 AM?"},
		{"chat.livepoy.0", "LIVE PLAYER OF THE YEAR!"},
		{"chat.livepoy.1", "the circuit's best, streaming for us?"},
		{"chat.livepoy.2", "live poy, chat. the real one!"},
		{"chat.teamriverline.0", "TEAM RIVERLINE?!"},
		{"chat.teamriverline.1", "the gold patch. big time now!"},
		{"chat.teamriverline.2", "signed by riverline. remember us when you're famous!"},
		{"chat.sponsored.0", "new patch, who dis?"},
		{"chat.sponsored.1", "sponsored?? fancy!"},
		{"chat.sponsored.2", "{Brand} on the hoodie. we made it!"},
		{"chat.card.0", "circuit card!!"},
		{"chat.card.1", "card holder now. the tour's calling!"},
		{"chat.card.2", "on the circuit's card. respect!"},
		{"chat.verified.0", "verified pro?? since when?"},
		{"chat.verified.1", "the hexagon is real!"},
		{"chat.verified.2", "VERIFIED. called it months ago!"},
		{"chat.kastpartner.0", "PARTNER!! the check is real!"},
		{"chat.kastpartner.1", "we did it, chat. we actually did it!"},
		{"chat.kastpartner.2", "purple check gang!"},
		{"chat.wall.0", "on the wall!!"},
		{"chat.wall.1", "frame it. the Embercrest's wall!"},
		{"chat.wall.2", "name on the wall, chat. we saw it live!"},
		{"chat.club.0", "new players club card?? look at that thing!"},
		{"chat.club.1", "the card room knows the name now!"},
		{"chat.club.2", "club card upgrade. the grind shows!"},
		// Honorifics (§5.5): one per name, in the intros and the studying caption.
		{"honor.hall", "Hall of Famer"},
		{"honor.bracelet", "bracelet winner"},
		{"honor.bracelet.n", "{Times} bracelet winner"},
		{"honor.ring", "ring winner"},
		{"honor.ring.n", "{Times} ring winner"},
		{"honor.poy", "last year's Player of the Year"},
		{"honor.livepoy", "last year's Live Player of the Year"},
		{"honor.teamriverline", "Team RiverLine's"},
		{"honor.embercrest", "Team Embercrest's own"},
		{"honor.crest", "a Crest member"},
		{"honor.wall", "Embercrest champion"},
		{"honor.wall.n", "{Times} Embercrest champion"},
		{"honor.card.gold", "a Gold Card holder"},
		{"honor.card.black", "a Black Card holder"},
		// The studying caption's words (§5.1: "Ember member · twice on the wall").
		{"cap.hall", "Hall of Famer"},
		{"cap.bracelet", "bracelet winner"},
		{"cap.bracelet.n", "{N} bracelets"},
		{"cap.ring", "ring winner"},
		{"cap.ring.n", "{N} rings"},
		{"cap.poy", "Player of the Year '{Yy}"},
		{"cap.livepoy", "Live Player of the Year '{Yy}"},
		{"cap.teamriverline", "Team RiverLine"},
		{"cap.sponsored", "sponsored by {Brand}"},
		{"cap.sponsored.bare", "sponsored"},
		{"cap.card.1", "Circuit Card '{Yy}"},
		{"cap.card.2", "Gold Card '{Yy}"},
		{"cap.card.3", "Black Card '{Yy}"},
		{"cap.verified", "verified pro"},
		{"cap.kastpartner", "Kast Partner"},
		{"cap.wall", "on the wall"},
		{"cap.wall.2", "twice on the wall"},
		{"cap.wall.n", "{Words} times on the wall"},
		{"cap.club.1", "Embercrest regular"},
		{"cap.club.2", "Ember member"},
		{"cap.club.3", "Crest member"},
		// Labels (small caps) and titles (the tooltip's first line).
		{"label.hall", "HALL OF FAME"},
		{"label.bracelet", "BRACELET"},
		{"label.bracelet.n", "{N}\xC3\x97 BRACELET"},
		{"label.ring", "RING"},
		{"label.ring.n", "{N}\xC3\x97 RING"},
		{"label.poy", "POY '{Yy}"},
		{"label.livepoy", "LIVE POY '{Yy}"},
		{"label.teamriverline", "TEAM RIVERLINE"},
		{"label.sponsored", "{BRAND}"},
		{"label.sponsored.bare", "SPONSORED"},
		{"label.card.1", "CIRCUIT CARD '{Yy}"},
		{"label.card.2", "GOLD CARD '{Yy}"},
		{"label.card.3", "BLACK CARD '{Yy}"},
		{"label.verified", "VERIFIED PRO"},
		{"label.kastpartner", "KAST PARTNER"},
		{"label.wall", "ON THE WALL"},
		{"label.wall.n", "ON THE WALL \xC3\x97{N}"},
		{"label.club.1", "REGULAR"},
		{"label.club.2", "EMBER"},
		{"label.club.3", "CREST"},
		{"title.hall", "Hall of Fame, class of {Year}"},
		{"title.bracelet", "Bracelet winner"},
		{"title.bracelet.n", "{TimesCap} bracelet winner"},
		{"title.ring", "Ring winner"},
		{"title.ring.n", "{TimesCap} ring winner"},
		{"title.poy", "Player of the Year {Year}"},
		{"title.livepoy", "Live Player of the Year {Year}"},
		{"title.teamriverline", "Team RiverLine"},
		{"title.sponsored", "Sponsored by {Brand}"},
		{"title.sponsored.bare", "Sponsored"},
		{"title.card.1", "Circuit Card {Year}"},
		{"title.card.2", "Gold Card {Year}"},
		{"title.card.3", "Black Card {Year}"},
		{"title.verified", "Verified Pro"},
		{"title.kastpartner", "Kast Partner"},
		{"title.wall", "On the Wall"},
		{"title.wall.n", "On the Wall \xC3\x97{N}"},
		{"title.club.1", "Players Club: Regular"},
		{"title.club.2", "Players Club: Ember"},
		{"title.club.3", "Players Club: Crest"},
		// Career's progress cards (badge::Next): a line and its detail.
		{"next.club", "{Tier} \xC2\xB7 {Have} of {Need} events in 90 days"},
		{"next.club.detail", "A night played counts once. Cards drop only on the 1st."},
		{"next.verified.events", "Verified Pro \xC2\xB7 {Have} of 1,000 tournaments"},
		{"next.verified.roi", "Verified Pro \xC2\xB7 ROI {Roi} of +15%"},
		{"next.verified.buyins", "Verified Pro \xC2\xB7 {Have} of 150 buy-ins up"},
		{"next.verified.played", "Verified Pro \xC2\xB7 no tournament in {Have} days"},
		{"next.verified.quiet", "Verified Pro \xC2\xB7 {Have} of 30 days clear"},
		{"next.verified.review", "Verified Pro \xC2\xB7 reviewed on the 1st"},
		{"next.verified.detail", "ROI {Roi} \xC2\xB7 {BuyIns} of 150 buy-ins up \xC2\xB7 {Quiet} of 30 days clear"},
		{"next.verified.shift", "Last ShiftLink shift: {Date}. It takes 30 days clear of the day job."},
		{"next.kast", "Kast Partner \xC2\xB7 {Have} of 75 average viewers"},
		{"next.kast.hours", "Kast Partner \xC2\xB7 {Have} of 25 hours live"},
		{"next.kast.days", "Kast Partner \xC2\xB7 {Have} of 12 days live"},
		{"next.kast.detail", "{Hours} of 25 hours \xC2\xB7 {Days} of 12 days \xC2\xB7 last 30 days"},
		{"next.card", "Circuit Card \xC2\xB7 {Have} of {Need} points"},
		{"next.card.early", "Circuit Card \xC2\xB7 standings from week 13"},
		{"next.card.detail", "Rank {Rank} \xC2\xB7 the top 200 hold a card. Regional events and up."},
		{"next.card.up", "{Tier} \xC2\xB7 {Have} of {Need} points"},
	};
	return Lines;
}

const std::string& Line(const char* Key)
{
	static const std::string None;
	const auto& Lines = Copy();
	const auto It = Lines.find(Key);
	return It == Lines.end() ? None : It->second;
}

std::string Fill(const std::string& Template, const std::map<std::string, std::string>& Vars)
{
	std::string Out;
	size_t I = 0;
	while (I < Template.size())
	{
		const size_t Open = Template.find('{', I);
		const size_t Close = Open == std::string::npos ? std::string::npos : Template.find('}', Open + 1);
		if (Close == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		Out.append(Template, I, Open - I);
		const auto It = Vars.find(Template.substr(Open + 1, Close - Open - 1));
		Out += It != Vars.end() ? It->second : Template.substr(Open, Close - Open + 1);
		I = Close + 1;
	}
	return Out;
}

std::string Say(const char* Key, const std::map<std::string, std::string>& Vars = {})
{
	return Fill(Line(Key), Vars);
}

std::string Upper(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return S;
}

std::string Capital(std::string S)
{
	if (!S.empty() && S[0] >= 'a' && S[0] <= 'z')
	{
		S[0] = static_cast<char>(S[0] - 'a' + 'A');
	}
	return S;
}

const char* NumberWord(int N)
{
	static const char* const Words[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"};
	return N >= 0 && N <= 12 ? Words[N] : nullptr;
}

/** "two-time", "thirteen-time" -> "13-time". */
std::string Times(int N)
{
	const char* W = NumberWord(N);
	return (W ? std::string(W) : std::to_string(N)) + "-time";
}

std::string TwoDigits(int Year)
{
	char Buf[8];
	std::snprintf(Buf, sizeof(Buf), "%02d", ((Year % 100) + 100) % 100);
	return Buf;
}

std::string LongDate(int Day)
{
	return net::DateLabel(Day) + ", " + std::to_string(world::YearOf(Day));
}

/** "+18%", "-3%". */
std::string Percent(double Roi)
{
	const long long P = std::llround(Roi * 100.0);
	return (P >= 0 ? "+" : "") + std::to_string(P) + "%";
}

/** The series a trophy was won at, as its page names it (the trophy case's TrophySeries). */
std::string SeriesName(const world::Award& A)
{
	if (A.Series.empty())
	{
		return A.Ring ? "Grand Circuit" : "The Championship";
	}
	if (A.Online)
	{
		const net::SeriesInfo* Sr = net::Shared().FindSeries(A.Series);
		return Sr ? Sr->Name : A.Series;
	}
	return A.Series;
}

std::string EventName(const world::Award& A)
{
	const size_t Colon = A.Event.find(": ");
	return Colon == std::string::npos ? A.Event : A.Event.substr(Colon + 2);
}

const char* KindKey(Kind K)
{
	switch (K)
	{
	case Kind::HallOfFame: return "hall";
	case Kind::Bracelet: return "bracelet";
	case Kind::Ring: return "ring";
	case Kind::PlayerOfYear: return "poy";
	case Kind::LivePlayerOfYear: return "livepoy";
	case Kind::TeamRiverLine: return "teamriverline";
	case Kind::Sponsored: return "sponsored";
	case Kind::CircuitCard: return "card";
	case Kind::VerifiedPro: return "verified";
	case Kind::KastPartner: return "kastpartner";
	case Kind::OnTheWall: return "wall";
	case Kind::Club: return "club";
	case Kind::Count: break;
	}
	return "";
}

/** The brand behind a sponsor field: an id (the player's) or a display name (the world's). */
const sponsor::Brand* BrandOf(const std::string& IdOrName)
{
	if (IdOrName.empty())
	{
		return nullptr;
	}
	const sponsor::Brand* B = sponsor::FindBrand(IdOrName);
	return B ? B : sponsor::FindBrandByName(IdOrName);
}

/** The site's own patch is Team RiverLine, never "Sponsored" (§1.9 #21). */
bool IsSitePatch(const std::string& IdOrName)
{
	const sponsor::Brand* B = BrandOf(IdOrName);
	return (B && B->Id == "riverline") || IdOrName == "RiverLine" || IdOrName == "Team RiverLine" || IdOrName == "riverline";
}

std::string BrandWords(const Badge& B)
{
	const sponsor::Brand* Br = sponsor::FindBrand(B.Brand);
	return Br ? Br->Short : B.Brand;
}

/** The colour a badge has by its kind, tier and brand (Pack leaves it off; a bracelet's enamel and a ring's stone are their own). */
uint32_t KindColor(const Badge& B)
{
	switch (B.K)
	{
	case Kind::HallOfFame: return 0xf2c14e;
	case Kind::Bracelet: return 0x2a2a35;
	case Kind::Ring: return 0x2f6fef;
	case Kind::PlayerOfYear: return 0x27d3c3;
	case Kind::LivePlayerOfYear: return 0xf6b04a;
	case Kind::TeamRiverLine: return 0xf2c14e;
	case Kind::Sponsored:
	{
		const sponsor::Brand* Br = sponsor::FindBrand(B.Brand);
		return Br ? Br->Color : 0x8d9ab4;
	}
	case Kind::CircuitCard: return B.Tier >= 3 ? 0x141416 : B.Tier == 2 ? 0xf2c14e : 0xc9d3e0;
	case Kind::VerifiedPro: return 0x27d3c3;
	case Kind::KastPartner: return 0x9b5cff;
	case Kind::OnTheWall: return 0xf2832e;
	case Kind::Club: return B.Tier >= 3 ? 0xf6b04a : B.Tier == 2 ? 0xd9531e : 0xebe7df;
	case Kind::Count: break;
	}
	return 0;
}

/** The titles history keeps for someone (-1: the player). */
std::vector<world::Honor> HonorsOf(const world::World& W, int Npc)
{
	std::vector<world::Honor> Out;
	for (const world::Honor& H : W.Honors())
	{
		if (H.Npc == Npc)
		{
			Out.push_back(H);
		}
	}
	return Out;
}

int MajorsIn(const std::vector<world::Honor>& Honors)
{
	int N = 0;
	for (const world::Honor& H : Honors)
	{
		N += H.Title != PoyTitle && H.Title != LivePoyTitle && H.Title != world::HallOfFameTitle ? 1 : 0;
	}
	return N;
}

Holder FromNpc(const world::Npc& N, int Day, std::vector<world::Honor> Honors)
{
	Holder H;
	H.Age = N.Age(Day);
	H.YearsOn = world::YearOf(Day) - world::YearOf(N.Joined);
	H.DaysSincePlayed = N.LastDay > -9999 ? std::max(0, Day - N.LastDay) : 9999;
	H.Professional = N.Professional;
	H.TeamRiverLine = N.Pro;
	H.KastPartner = N.Streams && N.Followers >= KastPartnerFollowers;
	if (!N.Sponsor.empty() && !IsSitePatch(N.Sponsor))
	{
		H.Sponsor = N.Sponsor;
		const sponsor::Brand* B = BrandOf(N.Sponsor);
		H.SponsorColor = B ? B->Color : 0x8d9ab4;
	}
	H.Stats = &N.Stats;
	H.Awards = &N.Awards;
	H.Majors = N.Majors;
	H.Honors = std::move(Honors);
	H.CardBest = N.CardBest;
	H.CardLast = N.CardLast;
	H.ClubVisitsNow = N.Room.Venue == "embercrest" ? VisitsIn(N.Room.Days, Day) : 0;
	H.ClubVisitsAtMonth = N.Room.Venue == "embercrest" ? VisitsIn(N.Room.Days, world::MonthStart(Day) - 1) : 0;
	H.WallWins = N.Room.Wins;
	H.WallLast = N.Room.LastWin;
	H.VerifiedHeld = N.Verified;
	return H;
}

/** What the world knows of the player (FieldLeaders: the card room's snapshot has no session). */
Holder FromWorldHero(const world::World& W, int Day)
{
	Holder H;
	H.Stats = &W.HeroStats();
	H.Awards = &W.HeroAwards();
	H.Honors = HonorsOf(W, -1);
	H.Majors = MajorsIn(H.Honors);
	H.CardBest = W.HeroCardBest();
	H.CardLast = W.HeroCardLast();
	const world::RoomRecord& R = W.HeroRoom();
	H.ClubVisitsNow = VisitsIn(R.Days, Day);
	H.ClubVisitsAtMonth = VisitsIn(R.Days, world::MonthStart(Day) - 1);
	H.WallWins = R.Wins;
	H.WallLast = R.LastWin;
	return H;
}

/**
 * What the world and the save know of the player: the trophies, titles and cards the world keeps; the live record's club
 * nights and wall; the running deals' patches; the last review's Verified. OfHero adds the session's own (the age, the
 * last tournament, Kast); the night's snapshot (HeroFieldPack) needs nothing more. No net::Shared(): the desk's worker
 * runs it too.
 */
Holder FromSave(const world::World& W, const life::State& L, double Now)
{
	const int Day = net::DayOf(Now);
	Holder H;
	// A professional: no ShiftLink shift in the last 30 days (§5.3).
	H.LastShiftAt = L.LastShiftAt;
	H.Professional = Now - L.LastShiftAt >= static_cast<double>(VerifiedQuietDays) * DayMinutes;
	// The patches: Team RiverLine, then the highest-tier brand of the running deals (sponsor::Badges, highest first).
	H.TeamRiverLine = sponsor::Active(L.Sponsors, "riverline", Now);
	for (const auto& Br : sponsor::Badges(L.Sponsors, Now))
	{
		if (Br.first != "riverline" && H.Sponsor.empty())
		{
			H.Sponsor = Br.first;
			H.SponsorColor = Br.second;
		}
	}
	H.Stats = &W.HeroStats();
	H.Awards = &W.HeroAwards();
	H.Honors = HonorsOf(W, -1);
	H.Majors = MajorsIn(H.Honors);
	H.CardBest = W.HeroCardBest();
	H.CardLast = W.HeroCardLast();
	// The room: the live record's days (a finished entry, one a day; a refund was never played) and the wall.
	const std::vector<int> Days = HeroRoomDays(L);
	H.ClubVisitsNow = VisitsIn(Days, Day);
	H.ClubVisitsAtMonth = VisitsIn(Days, world::MonthStart(Day) - 1);
	H.WallWins = L.LiveWins;
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State == life::LiveEntry::Finished && E.Place == 1 && live::IsEmbercrest(E.Id))
		{
			const live::Occurrence O = live::FindOccurrence(E.Id);
			const int On = O.Valid() ? O.Day : net::DayOf(E.FinishedAt);
			if (On >= H.WallLast)
			{
				H.WallLast = On;
				H.WallLastEvent = O.Valid() && O.T ? std::string(O.T->Short) : std::string();
			}
		}
	}
	if (H.WallLast < 0 && W.HeroRoom().LastWin >= 0)
	{
		H.WallLast = W.HeroRoom().LastWin;
	}
	H.VerifiedHeld = L.VerifiedHeld;
	return H;
}

/** The badges the room's snapshot keeps (§1.8: the plates, the studying caption, the intros, the leaders). A reigning
 *  Player of the Year never takes a seat's slot (§5.4's room order), but the floor reads it in an intro (§5.5). */
Set RoomOnly(const Set& S)
{
	Set Out;
	for (const Badge& B : S.Held)
	{
		const bool Keep = B.K == Kind::HallOfFame || B.K == Kind::Bracelet || B.K == Kind::Ring || B.K == Kind::OnTheWall || B.K == Kind::Club ||
			(B.K == Kind::CircuitCard && B.Tier >= world::CardGold) || B.K == Kind::TeamRiverLine || B.K == Kind::Sponsored || B.K == Kind::VerifiedPro ||
			((B.K == Kind::PlayerOfYear || B.K == Kind::LivePlayerOfYear) && B.Tier >= 1);
		if (Keep)
		{
			Out.Held.push_back(B);
		}
	}
	return Out;
}

bool PackSafe(char Ch)
{
	return (Ch >= '0' && Ch <= '9') || (Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z') || Ch == '_' || Ch == '-';
}

std::string Escape(const std::string& S)
{
	static const char* Hex = "0123456789ABCDEF";
	std::string Out;
	for (const char Ch : S)
	{
		if (PackSafe(Ch))
		{
			Out += Ch;
		}
		else
		{
			const unsigned char U = static_cast<unsigned char>(Ch);
			Out += '%';
			Out += Hex[U >> 4];
			Out += Hex[U & 15u];
		}
	}
	return Out;
}

int HexDigit(char Ch)
{
	return Ch >= '0' && Ch <= '9' ? Ch - '0' : Ch >= 'A' && Ch <= 'F' ? Ch - 'A' + 10 : Ch >= 'a' && Ch <= 'f' ? Ch - 'a' + 10 : -1;
}

std::string Unescape(const std::string& S)
{
	std::string Out;
	for (size_t I = 0; I < S.size(); ++I)
	{
		const int Hi = S[I] == '%' && I + 2 < S.size() ? HexDigit(S[I + 1]) : -1;
		const int Lo = Hi >= 0 ? HexDigit(S[I + 2]) : -1;
		if (Hi >= 0 && Lo >= 0)
		{
			Out += static_cast<char>(Hi * 16 + Lo);
			I += 2;
			continue;
		}
		Out += S[I];
	}
	return Out;
}

bool EndsWith(const std::string& S, const char* Tail)
{
	const std::string T(Tail);
	return S.size() >= T.size() && S.compare(S.size() - T.size(), T.size(), T) == 0;
}

/** An honorific that goes straight on the name ("Team RiverLine's Kenji", "Team Embercrest's own Mei"). */
bool Possessive(const std::string& Honor)
{
	return EndsWith(Honor, "'s") || EndsWith(Honor, " own");
}

std::vector<std::string> Split(const std::string& S, char Sep)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char Ch : S)
	{
		if (Ch == Sep)
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += Ch;
		}
	}
	Out.push_back(Cur);
	return Out;
}

int ToInt(const std::string& S, int Fallback = 0)
{
	if (S.empty())
	{
		return Fallback;
	}
	char* End = nullptr;
	const long V = std::strtol(S.c_str(), &End, 10);
	return End && *End == '\0' ? static_cast<int>(V) : Fallback;
}

/** A slot order's entries: a kind, and which of its tiers fill that slot (0: any). */
struct Slot
{
	Kind K;
	int MinTier;
	int MaxTier;
};

const std::vector<Slot>& OrderFor(uint8_t Context)
{
	// §5.4. Network: Hall › Bracelet › Ring › reigning POY › Team RiverLine › Sponsored › Black Card › Gold Card ›
	// Verified Pro › Kast Partner › Card. Room: Hall › Bracelet › Ring › On the Wall › Crest › Black/Gold Card ›
	// Team RiverLine › Sponsored › Ember › Verified Pro. Stream: Kast Partner › Team RiverLine › Sponsored › Hall ›
	// Bracelet › Ring. Card: everything, in the network's order, then the room's own.
	static const std::vector<Slot> NetworkOrder = {{Kind::HallOfFame, 0, 99}, {Kind::Bracelet, 0, 99}, {Kind::Ring, 0, 99}, {Kind::PlayerOfYear, 1, 99},
		{Kind::LivePlayerOfYear, 1, 99}, {Kind::TeamRiverLine, 0, 99}, {Kind::Sponsored, 0, 99}, {Kind::CircuitCard, 3, 3}, {Kind::CircuitCard, 2, 2},
		{Kind::VerifiedPro, 0, 99}, {Kind::KastPartner, 0, 99}, {Kind::CircuitCard, 1, 1}};
	static const std::vector<Slot> RoomOrder = {{Kind::HallOfFame, 0, 99}, {Kind::Bracelet, 0, 99}, {Kind::Ring, 0, 99}, {Kind::OnTheWall, 0, 99},
		{Kind::Club, 3, 3}, {Kind::CircuitCard, 2, 3}, {Kind::TeamRiverLine, 0, 99}, {Kind::Sponsored, 0, 99}, {Kind::Club, 2, 2}, {Kind::VerifiedPro, 0, 99}};
	static const std::vector<Slot> StreamOrder = {{Kind::KastPartner, 0, 99}, {Kind::TeamRiverLine, 0, 99}, {Kind::Sponsored, 0, 99}, {Kind::HallOfFame, 0, 99},
		{Kind::Bracelet, 0, 99}, {Kind::Ring, 0, 99}};
	static const std::vector<Slot> CardOrder = {{Kind::HallOfFame, 0, 99}, {Kind::Bracelet, 0, 99}, {Kind::Ring, 0, 99}, {Kind::PlayerOfYear, 0, 99},
		{Kind::LivePlayerOfYear, 0, 99}, {Kind::TeamRiverLine, 0, 99}, {Kind::Sponsored, 0, 99}, {Kind::CircuitCard, 3, 3}, {Kind::CircuitCard, 2, 2},
		{Kind::VerifiedPro, 0, 99}, {Kind::KastPartner, 0, 99}, {Kind::CircuitCard, 1, 1}, {Kind::OnTheWall, 0, 99}, {Kind::Club, 0, 99}};
	if ((Context & Card) != 0)
	{
		return CardOrder;
	}
	if ((Context & Room) != 0)
	{
		return RoomOrder;
	}
	if ((Context & Stream) != 0)
	{
		return StreamOrder;
	}
	return NetworkOrder;
}

/** C2: the frame already says it. A champion's frame shows the trophies (gold links for bracelets, with a ring winner's stone
 *  set on top; a ring's band and stone alone): neither the bracelet nor the ring takes the slot. The gold frame, Team RiverLine. */
bool FrameSays(const Badge& B, uint8_t Frame)
{
	if ((Frame & FrameChampion) != 0 && (B.K == Kind::Bracelet || B.K == Kind::Ring))
	{
		return true;
	}
	return (Frame & FrameGold) != 0 && B.K == Kind::TeamRiverLine;
}

/** One badge's words for the honorific ("" when it has none to say). */
std::string HonorOf(const Badge& B)
{
	switch (B.K)
	{
	case Kind::HallOfFame: return Say("honor.hall");
	case Kind::Bracelet: return B.Count >= 2 ? Say("honor.bracelet.n", {{"Times", Times(B.Count)}}) : Say("honor.bracelet");
	case Kind::Ring: return B.Count >= 2 ? Say("honor.ring.n", {{"Times", Times(B.Count)}}) : Say("honor.ring");
	case Kind::PlayerOfYear: return B.Tier >= 1 ? Say("honor.poy") : std::string();
	case Kind::LivePlayerOfYear: return B.Tier >= 1 ? Say("honor.livepoy") : std::string();
	case Kind::TeamRiverLine: return Say("honor.teamriverline");
	case Kind::Sponsored: return B.Brand == "embercrest" ? Say("honor.embercrest") : std::string();
	case Kind::CircuitCard: return B.Tier == 3 ? Say("honor.card.black") : B.Tier == 2 ? Say("honor.card.gold") : std::string();
	case Kind::OnTheWall: return B.Count >= 2 ? Say("honor.wall.n", {{"Times", Times(B.Count)}}) : Say("honor.wall");
	case Kind::Club: return B.Tier >= 3 ? Say("honor.crest") : std::string();
	default: return std::string();
	}
}

/** One badge in the caption's words. */
std::string CaptionOf(const Badge& B)
{
	switch (B.K)
	{
	case Kind::HallOfFame: return Say("cap.hall");
	case Kind::Bracelet: return B.Count >= 2 ? Say("cap.bracelet.n", {{"N", std::to_string(B.Count)}}) : Say("cap.bracelet");
	case Kind::Ring: return B.Count >= 2 ? Say("cap.ring.n", {{"N", std::to_string(B.Count)}}) : Say("cap.ring");
	case Kind::PlayerOfYear: return Say("cap.poy", {{"Yy", TwoDigits(B.Year)}});
	case Kind::LivePlayerOfYear: return Say("cap.livepoy", {{"Yy", TwoDigits(B.Year)}});
	case Kind::TeamRiverLine: return Say("cap.teamriverline");
	case Kind::Sponsored: return B.Brand.empty() ? Say("cap.sponsored.bare") : Say("cap.sponsored", {{"Brand", BrandWords(B)}});
	case Kind::CircuitCard: return Say(B.Tier >= 3 ? "cap.card.3" : B.Tier == 2 ? "cap.card.2" : "cap.card.1", {{"Yy", TwoDigits(B.Year)}});
	case Kind::VerifiedPro: return Say("cap.verified");
	case Kind::KastPartner: return Say("cap.kastpartner");
	case Kind::OnTheWall:
	{
		if (B.Count <= 1)
		{
			return Say("cap.wall");
		}
		if (B.Count == 2)
		{
			return Say("cap.wall.2");
		}
		const char* W = NumberWord(B.Count);
		return Say("cap.wall.n", {{"Words", W ? std::string(W) : std::to_string(B.Count)}});
	}
	case Kind::Club: return Say(B.Tier >= 3 ? "cap.club.3" : B.Tier == 2 ? "cap.club.2" : "cap.club.1");
	default: return std::string();
	}
}
} // namespace badge_detail

using namespace badge_detail;

const Badge* Set::Find(Kind K) const
{
	for (const Badge& B : Held)
	{
		if (B.K == K)
		{
			return &B;
		}
	}
	return nullptr;
}

uint32_t Set::Mask() const
{
	uint32_t Bits = 0;
	for (const Badge& B : Held)
	{
		Bits |= 1u << static_cast<uint32_t>(B.K);
	}
	return Bits;
}

// ------------------------------------------------------------------ who holds what

Holder Of(const world::World& W, int Npc, int Day)
{
	const world::Npc* N = W.Get(Npc);
	return N ? FromNpc(*N, Day, HonorsOf(W, Npc)) : Holder();
}

Holder OfHero(const Session& S)
{
	const world::World& W = S.Living();
	const double Now = S.WorldMinutes();
	const int Day = net::DayOf(Now);
	const life::State& L = S.Life;
	Holder H = FromSave(W, L, Now);
	const int StartYear = W.Ready() ? world::YearOf(net::DayOf(W.StartedAt())) : world::YearOf(net::NightOneDay);
	H.Age = S.Person.Age + std::max(0, world::YearOf(Day) - StartYear);
	H.YearsOn = std::max(0, world::YearOf(Day) - StartYear);
	// Played: the newest RiverLine tournament (its instance id carries its start) or the newest live day.
	int Last = -9999;
	for (const HistoryEntry& E : S.History)
	{
		const size_t At = E.EventId.rfind('@');
		if (At != std::string::npos && At + 1 < E.EventId.size())
		{
			const double Start = std::atof(E.EventId.c_str() + At + 1);
			if (Start > 0.0)
			{
				Last = std::max(Last, net::DayOf(Start));
			}
		}
	}
	if (!L.LiveDays.empty())
	{
		Last = std::max(Last, L.LiveDays.back().Day);
	}
	H.DaysSincePlayed = Last > -9999 ? std::max(0, Day - Last) : 9999;
	H.KastPartner = S.Channel.Partner;
	return H;
}

std::string HeroFieldPack(const world::World& W, const life::State& L, double World)
{
	// The room's badges only (no stream, no network titles but a reigning one), as the field's rows are packed.
	const int Day = net::DayOf(World);
	return Pack(RoomOnly(Earned(FromSave(W, L, World), Day)));
}

Set Earned(const Holder& H, int Day)
{
	Set S;
	const int Year = world::YearOf(Day);
	// The Hall: the class it was inducted with.
	for (const world::Honor& Hn : H.Honors)
	{
		if (Hn.Title == world::HallOfFameTitle)
		{
			Badge B;
			B.K = Kind::HallOfFame;
			B.Count = 1;
			B.Year = Hn.Year;
			B.Color = 0xf2c14e;
			S.Held.push_back(B);
			break;
		}
	}
	// Bracelets and rings: how many, the newest one's colour and year, a Main Event's among them.
	if (H.Awards)
	{
		for (int Ring = 0; Ring < 2; ++Ring)
		{
			Badge B;
			B.K = Ring ? Kind::Ring : Kind::Bracelet;
			const world::Award* Newest = nullptr;
			for (const world::Award& A : *H.Awards)
			{
				if (A.Ring != (Ring == 1))
				{
					continue;
				}
				++B.Count;
				B.Main = B.Main || A.Main;
				if (!Newest || A.Day >= Newest->Day)
				{
					Newest = &A;
				}
			}
			if (Newest)
			{
				B.Year = world::YearOf(Newest->Day);
				B.Color = ui::eventart::TrophyStone(*Newest);
				S.Held.push_back(B);
			}
		}
	}
	// Players of the Year: the newest title; reigning the year after it (the rows show it then; the card forever).
	for (int Live = 0; Live < 2; ++Live)
	{
		Badge B;
		B.K = Live ? Kind::LivePlayerOfYear : Kind::PlayerOfYear;
		const std::string Want = Live ? LivePoyTitle : PoyTitle;
		for (const world::Honor& Hn : H.Honors)
		{
			if (Hn.Title == Want)
			{
				++B.Count;
				B.Year = std::max(B.Year, Hn.Year);
			}
		}
		if (B.Count > 0)
		{
			B.Tier = B.Year == Year - 1 ? 1 : 0;
			B.Color = Live ? 0xf6b04a : 0x27d3c3;
			S.Held.push_back(B);
		}
	}
	if (H.TeamRiverLine)
	{
		Badge B;
		B.K = Kind::TeamRiverLine;
		B.Color = 0xf2c14e;
		B.Brand = "riverline";
		S.Held.push_back(B);
	}
	if (!H.Sponsor.empty() && !IsSitePatch(H.Sponsor))
	{
		Badge B;
		B.K = Kind::Sponsored;
		const sponsor::Brand* Br = BrandOf(H.Sponsor);
		B.Brand = Br ? Br->Id : H.Sponsor;
		B.Color = Br ? Br->Color : H.SponsorColor;
		S.Held.push_back(B);
	}
	// The circuit card: the best this season, or last season's (good through this one).
	const int CardTier = std::max(H.CardBest, H.CardLast);
	if (CardTier > world::CardNone)
	{
		Badge B;
		B.K = Kind::CircuitCard;
		B.Tier = std::min(CardTier, static_cast<int>(world::CardBlack));
		B.Year = H.CardBest >= H.CardLast ? Year : Year - 1;
		B.Color = B.Tier == 3 ? 0x141416 : B.Tier == 2 ? 0xf2c14e : 0xc9d3e0;
		S.Held.push_back(B);
	}
	if (H.VerifiedHeld)
	{
		Badge B;
		B.K = Kind::VerifiedPro;
		B.Color = 0x27d3c3;
		S.Held.push_back(B);
	}
	if (H.KastPartner)
	{
		Badge B;
		B.K = Kind::KastPartner;
		B.Color = 0x9b5cff;
		S.Held.push_back(B);
	}
	if (H.WallWins > 0)
	{
		Badge B;
		B.K = Kind::OnTheWall;
		B.Count = H.WallWins;
		B.Year = H.WallLast >= 0 ? world::YearOf(H.WallLast) : 0;
		B.Color = 0xf2832e;
		S.Held.push_back(B);
	}
	const int Club = ClubTier(H.ClubVisitsNow, H.ClubVisitsAtMonth);
	if (Club > 0)
	{
		Badge B;
		B.K = Kind::Club;
		B.Tier = Club;
		B.Count = H.ClubVisitsNow;
		B.Color = Club >= 3 ? 0xf6b04a : Club == 2 ? 0xd9531e : 0xebe7df;
		S.Held.push_back(B);
	}
	return S;
}

std::vector<Badge> InOrder(const Set& S, uint8_t Context, uint8_t Frame)
{
	std::vector<Badge> Out;
	for (const Slot& Sl : OrderFor(Context))
	{
		const Badge* B = S.Find(Sl.K);
		if (!B || B->Tier < Sl.MinTier || B->Tier > Sl.MaxTier || FrameSays(*B, Frame))
		{
			continue;
		}
		// C4: a Regular's card is never a glyph in rows (it stays the words "Embercrest regular"); only the full card shows it.
		if (B->K == Kind::Club && B->Tier < 2 && (Context & Card) == 0)
		{
			continue;
		}
		Out.push_back(*B);
	}
	return Out;
}

const Badge* Top(const Set& S, uint8_t Context, uint8_t Frame)
{
	for (const Slot& Sl : OrderFor(Context))
	{
		const Badge* B = S.Find(Sl.K);
		if (!B || B->Tier < Sl.MinTier || B->Tier > Sl.MaxTier || FrameSays(*B, Frame))
		{
			continue;
		}
		if (B->K == Kind::Club && B->Tier < 2 && (Context & Card) == 0)
		{
			continue;
		}
		return B;
	}
	return nullptr;
}

int ClubTier(int VisitsNow, int VisitsAtMonthStart)
{
	// Upgrades at once; a drop waits for the month's start: the tier is the better of now's and the 1st's.
	const int V = std::max(VisitsNow, VisitsAtMonthStart);
	return V >= ClubCrest ? 3 : V >= ClubEmber ? 2 : V >= ClubRegular ? 1 : 0;
}

int VisitsIn(const std::vector<int>& Days, int Day, int Window)
{
	std::vector<int> In;
	for (const int D : Days)
	{
		if (D > Day - Window && D <= Day)
		{
			In.push_back(D);
		}
	}
	std::sort(In.begin(), In.end());
	return static_cast<int>(std::unique(In.begin(), In.end()) - In.begin());
}

std::vector<int> HeroRoomDays(const life::State& L)
{
	std::vector<int> Days;
	for (const life::LiveDay& D : L.LiveDays)
	{
		if (D.Embercrest > 0)
		{
			Days.push_back(D.Day);
		}
	}
	return Days;
}

int HeroClubTier(const life::State& L, int Day)
{
	const std::vector<int> Days = HeroRoomDays(L);
	return ClubTier(VisitsIn(Days, Day), VisitsIn(Days, world::MonthStart(Day) - 1));
}

bool VerifiedNow(const Holder& H)
{
	return H.Stats && world::VerifiedRecord(*H.Stats, H.Professional, H.DaysSincePlayed);
}

double HallScore(const world::Npc& N, const world::World& W, int Year)
{
	return W.HallScore(N.Id, Year);
}

int CardFor(int Rank, int Week)
{
	if (Rank <= 0 || Week < CardFromWeek)
	{
		return world::CardNone;
	}
	return Rank <= CardTop[2] ? world::CardBlack : Rank <= CardTop[1] ? world::CardGold : Rank <= CardTop[0] ? world::CardRegular : world::CardNone;
}

// ------------------------------------------------------------------ the words

std::string Label(const Badge& B)
{
	const std::map<std::string, std::string> V = {{"N", std::to_string(B.Count)}, {"Yy", TwoDigits(B.Year)}, {"BRAND", Upper(BrandWords(B))}};
	switch (B.K)
	{
	case Kind::HallOfFame: return Say("label.hall");
	case Kind::Bracelet: return Say(B.Count >= 2 ? "label.bracelet.n" : "label.bracelet", V);
	case Kind::Ring: return Say(B.Count >= 2 ? "label.ring.n" : "label.ring", V);
	case Kind::PlayerOfYear: return Say("label.poy", V);
	case Kind::LivePlayerOfYear: return Say("label.livepoy", V);
	case Kind::TeamRiverLine: return Say("label.teamriverline");
	case Kind::Sponsored: return Say(B.Brand.empty() ? "label.sponsored.bare" : "label.sponsored", V);
	case Kind::CircuitCard: return Say(B.Tier >= 3 ? "label.card.3" : B.Tier == 2 ? "label.card.2" : "label.card.1", V);
	case Kind::VerifiedPro: return Say("label.verified");
	case Kind::KastPartner: return Say("label.kastpartner");
	case Kind::OnTheWall: return Say(B.Count >= 2 ? "label.wall.n" : "label.wall", V);
	case Kind::Club: return Say(B.Tier >= 3 ? "label.club.3" : B.Tier == 2 ? "label.club.2" : "label.club.1");
	case Kind::Count: break;
	}
	return std::string();
}

std::string Title(const Badge& B)
{
	const std::map<std::string, std::string> V = {{"N", std::to_string(B.Count)}, {"Year", std::to_string(B.Year)}, {"TimesCap", Capital(Times(B.Count))}, {"Brand", BrandWords(B)}};
	switch (B.K)
	{
	case Kind::HallOfFame: return Say("title.hall", V);
	case Kind::Bracelet: return Say(B.Count >= 2 ? "title.bracelet.n" : "title.bracelet", V);
	case Kind::Ring: return Say(B.Count >= 2 ? "title.ring.n" : "title.ring", V);
	case Kind::PlayerOfYear: return Say("title.poy", V);
	case Kind::LivePlayerOfYear: return Say("title.livepoy", V);
	case Kind::TeamRiverLine: return Say("title.teamriverline");
	case Kind::Sponsored: return Say(B.Brand.empty() ? "title.sponsored.bare" : "title.sponsored", V);
	case Kind::CircuitCard: return Say(B.Tier >= 3 ? "title.card.3" : B.Tier == 2 ? "title.card.2" : "title.card.1", V);
	case Kind::VerifiedPro: return Say("title.verified");
	case Kind::KastPartner: return Say("title.kastpartner");
	case Kind::OnTheWall: return Say(B.Count >= 2 ? "title.wall.n" : "title.wall", V);
	case Kind::Club: return Say(B.Tier >= 3 ? "title.club.3" : B.Tier == 2 ? "title.club.2" : "title.club.1");
	case Kind::Count: break;
	}
	return std::string();
}

std::string HowEarned(const Badge& B, const Holder& H)
{
	switch (B.K)
	{
	case Kind::HallOfFame: return Say("hover.hall", {{"Year", std::to_string(B.Year)}});
	case Kind::Bracelet:
	case Kind::Ring:
	{
		const bool Ring = B.K == Kind::Ring;
		const world::Award* Newest = nullptr;
		if (H.Awards)
		{
			for (const world::Award& A : *H.Awards)
			{
				if (A.Ring == Ring && (!Newest || A.Day >= Newest->Day))
				{
					Newest = &A;
				}
			}
		}
		const std::string Base = Ring ? "hover.ring" : "hover.bracelet";
		if (!Newest)
		{
			return Say((Base + (B.Count >= 2 ? ".manybare" : ".bare")).c_str(), {{"N", std::to_string(B.Count)}});
		}
		return Say((Base + (B.Count >= 2 ? ".many" : ".one")).c_str(),
			{{"N", std::to_string(B.Count)}, {"Event", EventName(*Newest)}, {"Series", SeriesName(*Newest)}, {"Date", LongDate(Newest->Day)}});
	}
	case Kind::PlayerOfYear:
	case Kind::LivePlayerOfYear:
	{
		const bool Live = B.K == Kind::LivePlayerOfYear;
		const std::string Want = Live ? LivePoyTitle : PoyTitle;
		const world::Honor* Won = nullptr;
		for (const world::Honor& Hn : H.Honors)
		{
			if (Hn.Title == Want && Hn.Year == B.Year)
			{
				Won = &Hn;
			}
		}
		const std::map<std::string, std::string> V = {{"Year", std::to_string(B.Year)}, {"Points", Won ? Grouped(Won->Entries) : std::string()}};
		return Say(Live ? (Won ? "hover.livepoy" : "hover.livepoy.bare") : (Won ? "hover.poy" : "hover.poy.bare"), V);
	}
	case Kind::TeamRiverLine: return Say("hover.teamriverline");
	case Kind::Sponsored: return B.Brand.empty() ? Say("hover.sponsored.bare") : Say("hover.sponsored", {{"Brand", BrandWords(B)}});
	case Kind::CircuitCard:
	{
		const int Tier = std::clamp(B.Tier, 1, 3);
		// A title this season puts its winner on the card whatever the points say: the line says which it was.
		const world::Award* Grant = nullptr;
		if (H.Awards)
		{
			for (const world::Award& A : *H.Awards)
			{
				if (world::YearOf(A.Day) == B.Year && (A.Main ? 2 : 1) >= Tier)
				{
					Grant = &A;
				}
			}
		}
		const std::map<std::string, std::string> V = {{"Top", std::to_string(CardTop[Tier - 1])}, {"Year", std::to_string(B.Year)}, {"Next", std::to_string(B.Year + 1)},
			{"Title", Grant ? std::string(Grant->Main ? "Main Event " : "") + (Grant->Ring ? "ring" : "bracelet") : std::string()}};
		const int Source = H.CardBest >= H.CardLast ? H.CardBest : H.CardLast;
		return Say(Grant && Source <= (Grant->Main ? 2 : 1) ? "hover.card.title" : "hover.card", V);
	}
	case Kind::VerifiedPro:
	{
		if (!H.Stats || H.Stats->PaidEvents() <= 0 || H.Stats->PaidAbi() <= 0)
		{
			return Say("hover.verified.bare");
		}
		const world::Tracker& T = *H.Stats;
		const long long Up = static_cast<long long>(T.PaidProfit() / T.PaidAbi());
		return Say("hover.verified", {{"Events", Grouped(T.PaidEvents())}, {"Roi", Percent(T.PaidRoi())}, {"BuyIns", Grouped(Up)}});
	}
	case Kind::KastPartner: return Say("hover.kast");
	case Kind::OnTheWall:
	{
		if (H.WallLast < 0)
		{
			return Say("hover.wall.bare");
		}
		const std::map<std::string, std::string> V = {{"Short", H.WallLastEvent}, {"Date", net::DateLabel(H.WallLast)}};
		return Say(H.WallLastEvent.empty() ? "hover.wall.date" : "hover.wall", V);
	}
	case Kind::Club:
	{
		const std::map<std::string, std::string> V = {{"N", std::to_string(B.Count)}};
		const int Need = B.Tier >= 3 ? ClubCrest : B.Tier == 2 ? ClubEmber : ClubRegular;
		if (B.Count < Need)
		{
			return Say("hover.club.held", V);
		}
		return Say(B.Tier >= 3 ? "hover.club.crest" : B.Tier == 2 ? "hover.club.ember" : "hover.club.regular", V);
	}
	case Kind::Count: break;
	}
	return std::string();
}

std::string Honorific(const Set& S, uint8_t Context)
{
	std::vector<Badge> Order = InOrder(S, Context, FrameNone);
	if ((Context & Room) != 0 && (Context & Card) == 0)
	{
		// The room's slot order leaves the network's titles off its seats, but the floor still reads a reigning Player of
		// the Year in an intro (§5.5): straight after the trophies, the live title first.
		auto At = std::find_if(Order.begin(), Order.end(), [](const Badge& B) { return B.K != Kind::HallOfFame && B.K != Kind::Bracelet && B.K != Kind::Ring; });
		for (const Kind K : {Kind::LivePlayerOfYear, Kind::PlayerOfYear})
		{
			const Badge* P = S.Find(K);
			if (P && P->Tier >= 1)
			{
				At = Order.insert(At, *P) + 1;
			}
		}
	}
	for (const Badge& B : Order)
	{
		const std::string H = HonorOf(B);
		if (!H.empty())
		{
			return H;
		}
	}
	return std::string();
}

std::string Introduce(const Set& S, uint8_t Context, const std::string& Name)
{
	const std::string H = Honorific(S, Context);
	if (H.empty())
	{
		return Name;
	}
	// A possessive goes straight on the name ("Team RiverLine's Kenji"); anything else is set off ("ring winner, Sal").
	return Possessive(H) ? H + " " + Name : H + ", " + Name;
}

std::string Caption(const Set& S, uint8_t Context, int Max)
{
	std::string Out;
	int N = 0;
	for (const Badge& B : InOrder(S, Context, FrameNone))
	{
		if (N >= Max)
		{
			break;
		}
		const std::string W = CaptionOf(B);
		if (W.empty())
		{
			continue;
		}
		Out += (Out.empty() ? std::string() : std::string(" \xC2\xB7 ")) + W;
		++N;
	}
	return Capital(Out);
}

std::string DeskLine(int Tier, bool Upgraded, const std::string& Addr)
{
	if (Tier <= 0)
	{
		return std::string();
	}
	const std::map<std::string, std::string> V = {{"Addr", Addr}};
	if (!Upgraded)
	{
		return Say("desk.kept", V);
	}
	return Say(Tier >= 3 ? "desk.crest" : Tier == 2 ? "desk.ember" : "desk.regular", V);
}

std::string ChatReaction(const Badge& B, uint32_t Salt)
{
	if (B.K == Kind::Count)
	{
		return std::string();
	}
	// A brand nobody knows by name has only the lines that don't name it.
	const uint32_t Lines = B.K == Kind::Sponsored && B.Brand.empty() ? 2u : 3u;
	const std::string Key = std::string("chat.") + KindKey(B.K) + "." + std::to_string(Salt % Lines);
	return Fill(Line(Key.c_str()), {{"Year", std::to_string(B.Year)}, {"Brand", BrandWords(B)}});
}

std::string SeenKey(const Badge& B)
{
	std::string Name = KindKey(B.K);
	int Tier = 0;
	int Count = 0;
	switch (B.K)
	{
	case Kind::HallOfFame: Count = B.Year; break;
	case Kind::Bracelet:
	case Kind::Ring:
	case Kind::PlayerOfYear:
	case Kind::LivePlayerOfYear:
	case Kind::OnTheWall: Count = B.Count; break;
	case Kind::Sponsored: Name += "." + Escape(B.Brand); break;
	case Kind::CircuitCard:
		Tier = B.Tier;
		Count = B.Year;
		break;
	case Kind::Club: Tier = B.Tier; break;
	default: break;
	}
	return "badge:" + Name + ":" + std::to_string(Tier) + ":" + std::to_string(Count);
}

std::string DeskKey(int Tier)
{
	return "badge-desk:club:" + std::to_string(Tier);
}

// ------------------------------------------------------------------ Career's progress cards

std::vector<Progress> Next(const Session& S, int Max)
{
	std::vector<Progress> Out;
	if (Max <= 0)
	{
		return Out;
	}
	const Holder H = OfHero(S);
	const int Day = net::DayOf(S.WorldMinutes());
	const Set& Held = S.HeroBadges();
	// The club: the next card up, counted the way the desk counts (a night played, once a day, the last 90 days).
	{
		const int Tier = ClubTier(H.ClubVisitsNow, H.ClubVisitsAtMonth);
		if (Tier < 3)
		{
			Progress P;
			P.K = Kind::Club;
			P.Tier = Tier + 1;
			P.Have = H.ClubVisitsNow;
			P.Need = Tier >= 2 ? ClubCrest : Tier == 1 ? ClubEmber : ClubRegular;
			const char* Name = Tier >= 2 ? "Crest" : Tier == 1 ? "Ember" : "Regular";
			P.Line = Say("next.club", {{"Tier", Name}, {"Have", std::to_string(H.ClubVisitsNow)}, {"Need", std::to_string(static_cast<int>(P.Need))}});
			P.Detail = Say("next.club.detail");
			Out.push_back(P);
		}
	}
	// Verified Pro: the first test the record doesn't pass yet, with the rest in the detail.
	if (!Held.Find(Kind::VerifiedPro) && H.Stats)
	{
		const world::Tracker& T = *H.Stats;
		const int Events = T.PaidEvents();
		const double Roi = T.PaidRoi();
		const Chips Abi = T.PaidAbi();
		const double Up = Abi > 0 ? static_cast<double>(T.PaidProfit()) / static_cast<double>(Abi) : 0.0;
		const double Quiet = std::max(0.0, std::floor((S.WorldMinutes() - H.LastShiftAt) / DayMinutes));
		const int QuietShown = static_cast<int>(std::min(Quiet, static_cast<double>(VerifiedQuietDays)));
		const int UpShown = static_cast<int>(std::floor(std::max(0.0, Up)));
		Progress P;
		P.K = Kind::VerifiedPro;
		if (Events < VerifiedEvents)
		{
			P.Have = Events;
			P.Need = VerifiedEvents;
			P.Line = Say("next.verified.events", {{"Have", Grouped(Events)}});
		}
		else if (!VerifiedNow(H) && T.PaidProfit() * 100 < static_cast<Chips>(std::llround(VerifiedRoi * 100.0)) * T.BuyIns)
		{
			P.Have = std::max(0.0, Roi);
			P.Need = VerifiedRoi;
			P.Line = Say("next.verified.roi", {{"Roi", Percent(Roi)}});
		}
		else if (!VerifiedNow(H) && Up < static_cast<double>(VerifiedBuyIns))
		{
			P.Have = std::max(0.0, Up);
			P.Need = VerifiedBuyIns;
			P.Line = Say("next.verified.buyins", {{"Have", std::to_string(UpShown)}});
		}
		else if (!H.Professional)
		{
			P.Have = Quiet;
			P.Need = VerifiedQuietDays;
			P.Line = Say("next.verified.quiet", {{"Have", std::to_string(QuietShown)}});
		}
		else if (H.DaysSincePlayed > world::VerifiedActiveDays)
		{
			P.Have = 0;
			P.Need = 1;
			P.Line = Say("next.verified.played", {{"Have", std::to_string(H.DaysSincePlayed)}});
		}
		else
		{
			P.Have = 1;
			P.Need = 1;
			P.Line = Say("next.verified.review");
		}
		P.Detail = H.LastShiftAt > -1.0e8 && !H.Professional
			? Say("next.verified.shift", {{"Date", net::DateLabel(net::DayOf(H.LastShiftAt))}})
			: Say("next.verified.detail", {{"Roi", Percent(Roi)}, {"BuyIns", std::to_string(UpShown)}, {"Quiet", std::to_string(QuietShown)}});
		Out.push_back(P);
	}
	// Kast Partner: once there's a channel to speak of (the player has streamed). The numbers are Kast's own 30-day window
	// (Channel::Window30, the one its Partner rule reads), so the card and the rule never disagree.
	if (!S.Channel.Partner && S.Channel.Streams > 0)
	{
		double Minutes = 0.0;
		int DaysN = 0;
		double Avg = 0.0;
		S.Channel.Window30(S.WorldMinutes(), Minutes, DaysN, Avg);
		const int Hours = static_cast<int>(std::floor(Minutes / 60.0));
		Progress P;
		P.K = Kind::KastPartner;
		if (Avg < kast::PartnerAvg)
		{
			P.Have = Avg;
			P.Need = kast::PartnerAvg;
			P.Line = Say("next.kast", {{"Have", std::to_string(static_cast<int>(std::floor(Avg)))}});
		}
		else if (Minutes < kast::PartnerMinutes)
		{
			P.Have = Minutes / 60.0;
			P.Need = kast::PartnerMinutes / 60.0;
			P.Line = Say("next.kast.hours", {{"Have", std::to_string(Hours)}});
		}
		else
		{
			P.Have = DaysN;
			P.Need = kast::PartnerDays;
			P.Line = Say("next.kast.days", {{"Have", std::to_string(DaysN)}});
		}
		P.Detail = Say("next.kast.detail", {{"Hours", std::to_string(Hours)}, {"Days", std::to_string(DaysN)}});
		Out.push_back(P);
	}
	// The circuit card: only once the player has circuit points (Act III: regional events and up).
	const world::World& W = S.Living();
	if (W.Ready() && (W.HeroCircuitPoints() > 0.0 || H.CardBest > 0 || H.CardLast > 0))
	{
		const int Week = world::SeasonWeek(Day);
		const int Best = H.CardBest;
		if (Best < world::CardBlack)
		{
			Progress P;
			P.K = Kind::CircuitCard;
			P.Tier = Best + 1;
			P.Have = std::floor(W.HeroCircuitPoints());
			const double Cut = W.CardCutLine(P.Tier);
			P.Need = std::max(P.Have, std::ceil(Cut));
			const char* Name = P.Tier >= 3 ? "Black Card" : P.Tier == 2 ? "Gold Card" : "Circuit Card";
			if (Week < CardFromWeek)
			{
				P.Line = Say("next.card.early");
			}
			else
			{
				P.Line = P.Tier == 1 ? Say("next.card", {{"Have", Grouped(static_cast<long long>(P.Have))}, {"Need", Grouped(static_cast<long long>(P.Need))}})
									 : Say("next.card.up", {{"Tier", Name}, {"Have", Grouped(static_cast<long long>(P.Have))}, {"Need", Grouped(static_cast<long long>(P.Need))}});
			}
			const int Rank = W.CircuitRank(-1);
			P.Detail = Say("next.card.detail", {{"Rank", Rank > 0 ? Grouped(Rank) : std::string("unranked")}});
			Out.push_back(P);
		}
	}
	if (static_cast<int>(Out.size()) > Max)
	{
		Out.resize(static_cast<size_t>(Max));
	}
	return Out;
}

// ------------------------------------------------------------------ the room's snapshot (live::DescribeField)

std::vector<std::pair<std::string, std::string>> FieldBadges(const world::World& W, const std::vector<int>& Field, int Day)
{
	// Straight from the rules, never Shared(): the desk's worker builds the field off the game thread.
	std::vector<std::pair<std::string, std::string>> Out;
	if (!W.Ready())
	{
		return Out;
	}
	for (const int Npc : Field)
	{
		const world::Npc* N = W.Get(Npc);
		if (!N)
		{
			continue;
		}
		const Set S = RoomOnly(Earned(FromNpc(*N, Day, HonorsOf(W, Npc)), Day));
		if (!S.Held.empty())
		{
			Out.push_back({N->Name, Pack(S)});
		}
	}
	return Out;
}

std::vector<life::RoomLeader> FieldLeaders(const world::World& W, int Day)
{
	std::vector<life::RoomLeader> Out;
	if (!W.Ready())
	{
		return Out;
	}
	int Rank = 0;
	for (const auto& Row : W.RoomLeaders("embercrest", world::MonthKeyOf(Day), 10))
	{
		life::RoomLeader L;
		L.Rank = ++Rank;
		L.Points = Row.second;
		if (Row.first < 0)
		{
			L.Name = W.HeroName;
			L.Badge = Pack(RoomOnly(Earned(FromWorldHero(W, Day), Day)));
		}
		else if (const world::Npc* N = W.Get(Row.first))
		{
			L.Name = N->Name;
			L.Badge = Pack(RoomOnly(Earned(FromNpc(*N, Day, HonorsOf(W, Row.first)), Day)));
		}
		Out.push_back(L);
	}
	return Out;
}

// ------------------------------------------------------------------ the index

const Set& Index::Of(const world::World& W, int Npc, int Day)
{
	static const Set Nothing{};
	if (!W.Ready() || Npc < 0 || Npc >= static_cast<int>(W.People().size()))
	{
		return Nothing;
	}
	if (For != &W || Seed != W.Seed() || Rev != W.Revision() || Built != Day || People != W.People().size())
	{
		For = &W;
		Seed = W.Seed();
		Rev = W.Revision();
		Built = Day;
		People = W.People().size();
		Builds = 0;
		Sets.assign(People, Set());
		Ready.assign(People, 0);
		HonorsBy.assign(People, std::vector<world::Honor>());
		for (const world::Honor& H : W.Honors())
		{
			if (H.Npc >= 0 && H.Npc < static_cast<int>(People))
			{
				HonorsBy[static_cast<size_t>(H.Npc)].push_back(H);
			}
		}
	}
	const size_t I = static_cast<size_t>(Npc);
	if (!Ready[I])
	{
		Sets[I] = Earned(FromNpc(W.People()[I], Day, HonorsBy[I]), Day);
		Ready[I] = 1;
		++Builds;
	}
	return Sets[I];
}

Index& Shared()
{
	static Index Built;
	return Built;
}

// ------------------------------------------------------------------ the packed form and the save

std::string Pack(const Set& S)
{
	// Each badge: kind.tier.count.year.main.colour.brand, trailing defaults left off; badges joined by ';'.
	std::string Out;
	for (const Badge& B : S.Held)
	{
		if (B.K == Kind::Count)
		{
			continue;
		}
		// The colour only where it isn't the kind's own (an enamel, a stone, a brand the catalog doesn't know).
		char Col[16];
		std::snprintf(Col, sizeof(Col), "%x", static_cast<unsigned>(B.Color & 0xffffffu));
		const bool OwnColor = B.Color != 0 && (B.Color & 0xffffffu) != KindColor(B);
		std::vector<std::string> F = {std::to_string(static_cast<int>(B.K)), std::to_string(B.Tier), std::to_string(B.Count), std::to_string(B.Year), B.Main ? "1" : "0",
			OwnColor ? std::string(Col) : std::string("0"), Escape(B.Brand)};
		const std::vector<std::string> Defaults = {"", "0", "0", "0", "0", "0", ""};
		while (F.size() > 1 && F.back() == Defaults[F.size() - 1])
		{
			F.pop_back();
		}
		std::string One;
		for (size_t K = 0; K < F.size(); ++K)
		{
			One += (K ? "." : "") + F[K];
		}
		Out += (Out.empty() ? "" : ";") + One;
	}
	return Out;
}

Set Unpack(const std::string& Packed)
{
	Set S;
	if (Packed.empty())
	{
		return S;
	}
	for (const std::string& One : Split(Packed, ';'))
	{
		const std::vector<std::string> F = Split(One, '.');
		const int K = ToInt(F[0], -1);
		if (K < 0 || K >= static_cast<int>(Kind::Count))
		{
			continue;
		}
		Badge B;
		B.K = static_cast<Kind>(K);
		B.Tier = F.size() > 1 ? ToInt(F[1]) : 0;
		B.Count = F.size() > 2 ? ToInt(F[2]) : 0;
		B.Year = F.size() > 3 ? ToInt(F[3]) : 0;
		B.Main = F.size() > 4 && F[4] == "1";
		B.Color = F.size() > 5 ? static_cast<uint32_t>(std::strtoul(F[5].c_str(), nullptr, 16)) & 0xffffffu : 0u;
		B.Brand = F.size() > 6 ? Unescape(F[6]) : std::string();
		B.Color = B.Color != 0 ? B.Color : KindColor(B);
		if (!S.Find(B.K))
		{
			S.Held.push_back(B);
		}
	}
	std::stable_sort(S.Held.begin(), S.Held.end(), [](const Badge& A, const Badge& B) { return static_cast<int>(A.K) < static_cast<int>(B.K); });
	return S;
}

void WriteLines(const life::State& L, std::string& Out)
{
	// badge\treview\t<monthKey>\t<verified>: written once a review has run (a save from before the segment has none).
	if (L.BadgeMonth >= 0)
	{
		Out += "badge\treview\t" + std::to_string(L.BadgeMonth) + "\t" + (L.VerifiedHeld ? "1" : "0") + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& Fields, life::State& L)
{
	if (Fields.size() < 2 || Fields[0] != "badge")
	{
		return false;
	}
	if (Fields[1] == "review" && Fields.size() >= 4)
	{
		L.BadgeMonth = ToInt(Fields[2], -1);
		L.VerifiedHeld = Fields[3] == "1";
	}
	// Any other badge line (a later build's) is ours to skip.
	return true;
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	// Every sentence the badges say: hover lines, desk lines, chat; the honorifics as the floor says them in an intro.
	std::vector<std::pair<std::string, std::string>> Out;
	for (const auto& Kv : Copy())
	{
		const std::string& K = Kv.first;
		if (K.rfind("hover.", 0) == 0 || K.rfind("desk.", 0) == 0 || K.rfind("chat.", 0) == 0)
		{
			Out.push_back(Kv);
		}
		else if (K.rfind("honor.", 0) == 0)
		{
			const std::string Said = Fill(Kv.second, {{"Times", "two-time"}});
			Out.push_back({K, "In seat three, " + Said + (Possessive(Said) ? " " : ", ") + "{Name}."});
		}
		else if (K == "next.club.detail" || K == "next.verified.shift")
		{
			Out.push_back(Kv);
		}
	}
	return Out;
}

std::vector<std::pair<std::string, std::string>> Labels()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const auto& Kv : Copy())
	{
		const std::string& K = Kv.first;
		if (K.rfind("label.", 0) == 0 || K.rfind("title.", 0) == 0 || K.rfind("cap.", 0) == 0 || (K.rfind("next.", 0) == 0 && K != "next.club.detail" && K != "next.verified.shift"))
		{
			Out.push_back(Kv);
		}
	}
	return Out;
}
} // namespace badge
} // namespace ss
