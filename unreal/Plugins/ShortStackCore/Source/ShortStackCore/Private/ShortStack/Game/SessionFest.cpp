// Update 4, the session's side of Festival Season (SPEC v1.7 §2.8.5, §2.9): the champion's, the belt's, a mark's and a title's
// moments, FestStep's texts, letters, crown tickets and refunds, Dee's lines and Rerun's titles (fest::TextInventory) and ss.Fest.*.
// F-world's file from G1.
//
// Phase A (§6 PA-37; the lead): stubs that keep today's behaviour. No festival exists (fest::All() is empty), so no moment waits,
// FestStep (CheckCalendar's, after SitePlayStep) has nothing to send and ss.Fest.* answers that it isn't built yet.
// G1's FW-1 (F-world): the festival texts' one table and fest::TextInventory over it (below). FW-5: the session's texts and moments
// (SessionFestDetail.h: ShowResults' festival branch and the seat line, T12's fill, the cap, the marks' moments, Rerun's festival
// stories), PeekFestMoment, DoneFestMoment and FestStep's belt and mark texts. FW-6: FestStep's crown letter (L5-L6), crown tickets
// (festticket:<key>) and step refunds (festcredit:<ticket>), and ss.Fest.* (DebugFest: describe, calendar, race, hub, look, and the dev
// verbs moment, pass and result).
#include "SessionFestDetail.h"
#include "SitesDetail.h"
#include "WorldFestDetail.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace video
{
namespace video_copy
{
const std::vector<std::pair<std::string, std::string>>& FestStoryTitles(); // G/VideoCopy.cpp: §2.11's V1-V9, (key prefix, title)
} // namespace video_copy
} // namespace video

namespace fest
{
namespace fest_text_detail
{
/** One festival text (§2.11): its key, its kind (the copy lint's stop rule and length cap read it, PLAN_G1 §1 item 18: "text" a
 *  phone text, "label" no closing stop, "journey" a sentence of at most 120 characters) and its template. */
struct FestTextLine
{
	const char* Key;
	const char* Kind;
	const char* Text;
};

/** The festival texts (§2.9.3, §2.11): Dee's lines, the two existing lines G/Session.cpp prints from here (T12 with its {Step1},
 *  T13), WHO'S IN's championship reasons (FestWhyLine composes them, LEAD_CALLS R-16, R-21) and the championship pair's story line
 *  (its beat is W-story's at G2, §2.6.7). One table: the session prints these templates and fest::TextInventory lists them. */
const FestTextLine FestTextTable[] = {
	// A festival Main won, by brand (key fest-main-<series id>): T1 RCOP, T2 FEIGN, T3 PUSH, T4 WRIT, T5 GRIT, T6 Summer Slam,
	// T7 Ring Rush, T8 the Championship Online, T15 PENNANT, T16 REIGN (the one "Long may you reign" in the game, §2.11).
	{"T1", "text", "You won the RCOP Main?? The Cup goes on the shelf. The shelf goes in the will."},
	{"T2", "text", "You won the FEIGN Main?? Okay. Put the mirror somewhere I can see it."},
	{"T3", "text", "You won the PUSH Main?? The Big Red Button is yours. Don't press it indoors."},
	{"T4", "text", "The Seal. You beat the high rollers at their own game, kid."},
	{"T5", "text", "The Grindstone! All that grinding, and you took the Main. Take a bow."},
	{"T6", "text", "The Slam Plate! Eat dinner off it once. Just once."},
	{"T7", "text", "The Ring Rush Main. That's the ring they all want."},
	{"T8", "text", "The Championship Online Main Event. A bracelet AND the title. Sit down a minute."},
	{"T15", "text", "You won the PENNANT Main?? Hang that flag where the neighbours can see it."},
	{"T16", "text", "The Sceptre, kid. Long may you reign. Mind the lampshade when you wave it."},
	// A Main's final table, 2nd to 9th (fest-ft-<series id>); the belt (fest-belt-<series id>); a seat into an edition's Main by its
	// Main ticket (seat-<ticket>); FINALIST or PODIUM (fest-mark-<series id>); a championship's Mini or Micro title (T17).
	{"T9", "text", "Final table of the {Festival} Main Event. Whatever happens next, you were there."},
	{"T10", "text", "There's a belt. You have to wear the belt."},
	{"T11", "text", "You won a seat in the {Festival} Main Event. {MainBuyIn} of tournament for a {SatBuyIn} ticket. Kid."},
	{"T14", "text", "{MARK} at {Festival}. {Rank} of {Scorers} in the race. I'm telling people."},
	{"T17", "text", "The {Festival} {Tier} Main. A title is a title, kid. Frame something."},
	// Existing lines, unchanged: gh0stfold's after a Micro Madness title (T12; {Step1} is the RCOP ladder's first step, §2.2.2) and
	// RiverLine's when satellites open (T13).
	{"T12", "text", "gg on the title. enjoy it. the RCOP satellites start at {Step1}, see you on the steps"},
	{"T13", "text", "New on your account: satellites. Win your way up the steps to the RCOP Main Event."},
	// WHO'S IN's championship reasons (§2.6.7, §4.7.3).
	{"who:fest:defseries", "label", "DEFENDING PLAYER OF THE SERIES \xC2\xB7 {Name} \xC2\xB7 {Points} pts"},
	{"who:fest:titles", "label", "{Name} \xC2\xB7 {N} {Festival} titles"},
	{"who:fest:yearchamp", "label", "{Name} \xC2\xB7 {Festival} {Year} champion"},
	// The championship pair's story line, and the sentence it gains while a third championship of the year is still to run.
	{"story:champpair", "journey", "{Name} won the {A} Main Event in {MonthA} and the {B} Main Event in {MonthB}."},
	{"story:champpair:next", "journey", "{C} opens {Date}."},
};

/** A festival text's template by its key ("T1" to "T17", "who:fest:titles", ...); "" for a key the table doesn't hold. */
const char* FestText(const std::string& Key)
{
	for (const FestTextLine& L : FestTextTable)
	{
		if (Key == L.Key)
		{
			return L.Text;
		}
	}
	return "";
}
} // namespace fest_text_detail

std::vector<std::string> TextInventory()
{
	// Every entry as "<kind>\t<text>" (the G/VideoCopy.cpp precedent, §2.11): the table, the story line's long form (the pair with its
	// next championship: one printed form) and Rerun's festival titles (V1-V9, G/VideoCopy.cpp's one table).
	using namespace fest_text_detail;
	std::vector<std::string> Out;
	for (const FestTextLine& L : FestTextTable)
	{
		if (std::string(L.Key) == "story:champpair:next")
		{
			continue;
		}
		Out.push_back(std::string(L.Kind) + "\t" + L.Text);
		if (std::string(L.Key) == "story:champpair")
		{
			Out.push_back(std::string(L.Kind) + "\t" + L.Text + " " + FestText("story:champpair:next"));
		}
	}
	for (const std::pair<std::string, std::string>& V : video::video_copy::FestStoryTitles())
	{
		Out.push_back("label\t" + V.second);
	}
	return Out;
}
} // namespace fest

namespace session_fest_detail
{
namespace
{
/** Dee's line for a festival Main won, by brand (§2.11: T1-T8, T15, T16); "" for None. */
const char* FestMainTextKey(fest::Brand B)
{
	switch (B)
	{
	case fest::Brand::Rcop: return "T1";
	case fest::Brand::Feign: return "T2";
	case fest::Brand::Push: return "T3";
	case fest::Brand::Writ: return "T4";
	case fest::Brand::Grit: return "T5";
	case fest::Brand::Slam: return "T6";
	case fest::Brand::RingRush: return "T7";
	case fest::Brand::Tco: return "T8";
	case fest::Brand::Pennant: return "T15";
	case fest::Brand::Reign: return "T16";
	case fest::Brand::None:
	case fest::Brand::Count: break;
	}
	return "";
}

/** The four keys that fill the cap's second slot for an edition (§2.9.3: the win line, a championship title's T17 counting as one,
 *  the final-table line and the mark line). */
const char* const FestWinPrefixes[] = {"fest-main-", "fest-title-", "fest-ft-", "fest-mark-"};

/** Rerun's three story prefixes (§2.9.4), in V1-V9's order (G/VideoCopy.cpp's FestStoryTitles). */
const char* const FestStoryPrefixes[] = {"fest-main-", "fest-belt-", "fest-ft-"};

/** A template's buy-in (0 when the template is unknown). */
Chips FestBuyInOf(const std::string& TemplateId)
{
	const net::EventTemplate* T = net::ForEvent(TemplateId).FindTemplate(TemplateId);
	return T ? T->BuyInCents : 0;
}

/** ASCII upper case (the trophy as V2 shouts it). */
std::string FestUpper(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return S;
}

/** Dee's ladder line (today's, G/Session.cpp's seat branch; one literal, FestLadderLine). */
const char* const FestLadderText = "A ticket up the ladder. Keep climbing.";

/** Id ends with Tail. */
bool FestIdEnds(const std::string& Id, const char* Tail)
{
	const size_t N = std::strlen(Tail);
	return Id.size() >= N && Id.compare(Id.size() - N, N, Tail) == 0;
}

/** An edition's name without its year ("RCOP 2027" -> "RCOP", "The Championship Online 2027" -> "The Championship Online"): L5's and
 *  L6's {Festival}, which their {Year} follows. */
std::string FestNameNoYear(const fest::Festival& F)
{
	const std::string Tail = " " + std::to_string(F.Year);
	return FestIdEnds(F.Name, Tail.c_str()) && F.Name.size() > Tail.size() ? F.Name.substr(0, F.Name.size() - Tail.size()) : F.Name;
}

/** The brand's first edition after F (fest::All() is in FirstDay order); nullptr after its last. */
const fest::Festival* FestNextEdition(const fest::Festival& F)
{
	for (const fest::Festival& G : fest::All())
	{
		if (G.Which == F.Which && G.FirstDay > F.FirstDay)
		{
			return &G;
		}
	}
	return nullptr;
}

/** FestCrownTicket's rule, with Event set to the template whose name the ticket's label carries (the next edition's Main, or the
 *  satellite). */
std::string FestCrownSeat(const fest::Festival& F, int Mark, std::string& Event)
{
	Event.clear();
	const bool Earns = Mark == static_cast<int>(world::FestMark::Title) || Mark == static_cast<int>(world::FestMark::Podium) ||
		Mark == static_cast<int>(world::FestMark::Finalist);
	const fest::Festival* Next = Earns ? FestNextEdition(F) : nullptr;
	if (!Next)
	{
		return std::string();
	}
	const net::Network& N = net::ForEvent(Next->MainId);
	if (Mark == static_cast<int>(world::FestMark::Title))
	{
		Event = Next->MainId;
		return N.TicketOf(Next->MainId); // a championship's year-free Main ticket, RCOP's rcop-main, any other edition's Main
	}
	std::string Mega, Last;
	for (const std::string& Id : Next->Satellites)
	{
		Mega = FestIdEnds(Id, "-sat1") || FestIdEnds(Id, "-mega") || Id == "mega-rcop" ? Id : Mega;
		Last = FestIdEnds(Id, "-sat2") || FestIdEnds(Id, "-last") || Id == "step4" ? Id : Last;
	}
	Event = Mark == static_cast<int>(world::FestMark::Podium) || Last.empty() ? Mega : Last; // WRIT: The Summons for both
	return Event.empty() ? std::string() : N.TicketOf(Event);
}

/** A ticket's face for the Governor: what a seat into it is worth (Network::SeatValue), else its event's buy-in (a Mega or a Last
 *  Chance: nothing seats into those). */
Chips FestTicketFace(const std::string& Ticket)
{
	const net::Network& N = net::ForEvent(Ticket);
	const Chips Seat = N.SeatValue(Ticket);
	const net::EventTemplate* T = Seat > 0 ? nullptr : N.FindTemplate(Ticket);
	return Seat > 0 ? Seat : T ? T->BuyInCents : 0;
}

/** The step a championship ticket enters (item 8's {K}): 1 + the distinct tickets before it in the edition's ladder order (Step 1's two
 *  templates take one ticket, -s1a; Step 2's -s2a; Step 3's -s3a); 0 when none of F's satellites takes it. Seated: one of F's
 *  satellites seats it and it is not the Main ticket (a step ticket, FestStepCredits). */
int FestStepOf(const fest::Festival& F, const std::string& Ticket, bool& Seated)
{
	const net::Network& N = net::ForEvent(F.MainId);
	const std::string Main = N.TicketOf(F.MainId);
	std::vector<std::string> Seen;
	int Step = 0;
	Seated = false;
	for (const std::string& Id : F.Satellites)
	{
		const std::string Takes = N.TicketOf(Id);
		if (std::find(Seen.begin(), Seen.end(), Takes) == Seen.end())
		{
			Seen.push_back(Takes);
		}
		Step = Step == 0 && Takes == Ticket ? static_cast<int>(Seen.size()) : Step;
		const net::EventTemplate* T = N.FindTemplate(Id);
		Seated = Seated || (T && T->SeatTicket == Ticket && Ticket != Main);
	}
	return Step;
}

/** The edition a year-free championship Main ticket ("ignite.pennant-main") is weighed against: the latest edition of its brand the
 *  world has crowned by Now whose Main takes it; nullptr for any other ticket. Its Main is never credited (FestStepOf: not Seated). */
const fest::Festival* FestMainTicketEdition(const std::string& Ticket, const world::World& W, double Now)
{
	if (!FestIdEnds(Ticket, "-main") || net::SiteOf(Ticket) == net::SiteId::RiverLine)
	{
		return nullptr;
	}
	const std::vector<fest::Festival>& All = fest::All();
	for (auto It = All.rbegin(); It != All.rend(); ++It)
	{
		const double Crown = fest::IsChampionship(It->Which) && net::ForEvent(It->MainId).TicketOf(It->MainId) == Ticket ? world::FestCrownedAt(W, It->SeriesId) : -1.0;
		if (Crown >= 0.0 && Crown <= Now)
		{
			return &*It;
		}
	}
	return nullptr;
}
} // namespace

FestResult FestResultOf(const std::string& EventId, int Place, bool SeatWon, Chips Prize, const std::set<std::string>& TextsSeen)
{
	FestResult Out;
	const size_t At = EventId.find('@');
	if (At == std::string::npos || Place < 1 || SeatWon)
	{
		return Out;
	}
	const std::string Template = EventId.substr(0, At);
	const net::EventTemplate* Tmpl = net::ForEvent(EventId).FindTemplate(Template);
	const fest::Festival* F = Tmpl ? fest::FestivalOf(Tmpl->Series) : nullptr;
	if (!F || std::find(F->Satellites.begin(), F->Satellites.end(), Template) != F->Satellites.end())
	{
		return Out; // not a festival's, or a satellite (never a moment, never a festival line)
	}
	const bool Main = Template == F->MainId;
	// A championship's other two titles, TitleIds' order {main, mini, micro} (§2.6.2; RCOP's Mini and Micro too).
	int Tier = 0;
	for (size_t I = 1; !Main && fest::IsChampionship(F->Which) && I < F->TitleIds.size() && I <= 2; ++I)
	{
		Tier = F->TitleIds[I] == Template ? static_cast<int>(I) : Tier;
	}
	const std::string& Sid = F->SeriesId;
	const std::string Name = fest::NameAfterArticle(F->Name); // every line below writes "the {Festival}"
	FestMoment M;
	M.Which = F->Which;
	M.Key = Sid;
	M.Prize = Prize;
	M.Year = F->Year;
	if (Main && Place == 1)
	{
		Out.Branch = true;
		Out.Key = "fest-main-" + Sid;
		Out.Body = fest::fest_text_detail::FestText(FestMainTextKey(F->Which));
		M.Kind = 1;
	}
	else if (Tier > 0 && Place == 1)
	{
		Out.Branch = true;
		Out.Key = "fest-title-" + Sid;
		Out.Body = site::FillCopy(fest::fest_text_detail::FestText("T17"), {{"Festival", Name}, {"Tier", Tier == 1 ? "Mini" : "Micro"}});
		M.Kind = 5;
		M.Mark = Tier;
	}
	else if (Main && Place <= 9)
	{
		Out.Branch = true;
		Out.Key = "fest-ft-" + Sid;
		Out.Body = site::FillCopy(fest::fest_text_detail::FestText("T9"), {{"Festival", Name}});
	}
	else if (Place == 1)
	{
		M.Kind = 3; // a numbered event or a headliner: the champion's moment, no festival line (today's lines follow)
	}
	if (Out.Branch && FestWinLineSent(TextsSeen, Sid))
	{
		Out.Key.clear(); // the cap: this edition's win, title, final-table or mark line already went
		Out.Body.clear();
	}
	if (M.Kind > 0)
	{
		Out.Moment = M;
	}
	return Out;
}

std::string FestSeatText(const std::string& EventId, const std::string& SeatTicket, Chips SatBuyInCents)
{
	if (SeatTicket.empty() || SeatTicket == "rcop-main")
	{
		return ""; // RCOP's Main keeps today's line (§2.2.2)
	}
	// The edition whose Main the ticket enters next (fest::All() is in FirstDay order; a championship's Main ticket serves every year).
	const size_t At = EventId.find('@');
	const double From = At == std::string::npos ? 0.0 : std::atof(EventId.c_str() + At + 1);
	for (const fest::Festival& F : fest::All())
	{
		if (F.MainStart < From || net::ForEvent(F.MainId).TicketOf(F.MainId) != SeatTicket)
		{
			continue;
		}
		return site::FillCopy(fest::fest_text_detail::FestText("T11"),
			{{"Festival", fest::NameAfterArticle(F.Name)}, {"MainBuyIn", net::BuyIn(FestBuyInOf(F.MainId))}, {"SatBuyIn", net::BuyIn(SatBuyInCents)}});
	}
	return ""; // a step ticket (a championship's Step 1 or 2), or any other seat: the ladder line
}

std::string FestStep1Fill()
{
	const net::EventTemplate* Step1 = net::Site(net::SiteId::RiverLine).FindTemplate("step1");
	return Step1 ? net::BuyIn(Step1->BuyInCents) : std::string();
}

bool FestWinLineSent(const std::set<std::string>& TextsSeen, const std::string& SeriesId)
{
	for (const char* Prefix : FestWinPrefixes)
	{
		if (TextsSeen.count(Prefix + SeriesId) > 0)
		{
			return true;
		}
	}
	return false;
}

std::optional<FestMoment> FestMarkMoment(const world::World& W, const std::set<std::string>& Done)
{
	// The belt's first, then the marks' (§2.8.5: "after the belt's, if both"), each in crown order.
	for (int Pass = 0; Pass < 2; ++Pass)
	{
		for (const world::HeroFestMark& K : world::HeroMarks(W))
		{
			const bool Belt = K.Mark == world::FestMark::Title;
			if ((Pass == 0) != Belt || K.Mark == world::FestMark::None || K.Mark >= world::FestMark::Count)
			{
				continue;
			}
			const fest::Festival* F = fest::FestivalOf(K.Key);
			if (!F || Done.count((Belt ? "festbelt:" : "festmark:") + K.Key) > 0)
			{
				continue;
			}
			FestMoment M;
			M.Kind = Belt ? 2 : 4;
			M.Which = F->Which;
			M.Key = K.Key;
			M.Prize = static_cast<Chips>(std::llround(K.Points));
			M.Year = F->Year;
			M.Mark = static_cast<int>(K.Mark);
			M.Rank = K.Rank;
			M.Scorers = K.Scorers;
			return M;
		}
	}
	return std::nullopt;
}

int FestStoryPrefix(const std::string& Key)
{
	for (int I = 0; I < 3; ++I)
	{
		if (Key.rfind(FestStoryPrefixes[I], 0) == 0)
		{
			return I;
		}
	}
	return -1;
}

std::vector<std::pair<std::string, std::string>> FestStoryFacts(const world::World& W, const std::string& Key)
{
	std::vector<std::pair<std::string, std::string>> Out;
	const int P = FestStoryPrefix(Key);
	const fest::Festival* F = P >= 0 ? fest::FestivalOf(Key.substr(std::string(FestStoryPrefixes[P]).size())) : nullptr;
	if (!F)
	{
		return Out;
	}
	// The whole name (§2.11's articles rule): V1, V4 and V7-V9 write "the {Festival}", so their fill drops a leading "The "; V3 and V6
	// print it whole (G/VideoCopy.cpp's FestStoryNameFill, in Update 2's titles and Rerun 2.0's).
	Out.emplace_back("Festival", F->Name);
	if (P == 0)
	{
		Out.emplace_back("TROPHY", FestUpper(fest::TrophyName(F->Which, 0)));
		Out.emplace_back("BuyIn", net::BuyIn(FestBuyInOf(F->MainId)));
	}
	else if (P == 1)
	{
		int Events = 0;
		for (const world::HeroFestPin& Pin : world::HeroFestPins(W))
		{
			Events = Pin.Key == F->SeriesId ? Pin.Events : Events;
		}
		Out.emplace_back("N", Grouped(Events));
	}
	else
	{
		int Place = 0;
		for (const world::HeroFestResult& R : world::HeroFestResults(W, F->SeriesId))
		{
			const bool MainRow = R.EventId.compare(0, R.EventId.find('@'), F->MainId) == 0;
			Place = Place == 0 && MainRow ? R.Place : Place; // newest first
		}
		if (Place > 0)
		{
			Out.emplace_back("Place", Ordinal(Place));
		}
	}
	return Out;
}

const char* FestLadderLine()
{
	return FestLadderText;
}

std::string FestCrownTicket(const fest::Festival& F, int Mark)
{
	std::string Event;
	return FestCrownSeat(F, Mark, Event);
}

bool FestPayCrownTicket(life::State& L, world::World& W, const world::HeroFestMark& K, double Now)
{
	// §2.6.1 step 7: once, keyed festticket:<key> in the sites' book (the site\tdone line, never pruned: no '@'); the mark's t drops
	// with it (FestTicketPaid), so a reload neither pays again nor offers it again.
	const std::string Key = "festticket:" + K.Key;
	const std::string MarkKey = K.Key; // K is the world's own row: FestTicketPaid writes it
	if (!fest::kCrownTickets || !K.Ticket)
	{
		return false;
	}
	const fest::Festival* F = fest::FestivalOf(MarkKey);
	if (!F || L.Sites.Done.count(Key) > 0)
	{
		world::FestTicketPaid(W, MarkKey); // paid before (or no festival): only the world's t is left to drop
		return true;
	}
	std::string Event;
	const std::string Ticket = FestCrownSeat(*F, static_cast<int>(K.Mark), Event);
	if (!Ticket.empty())
	{
		const net::SiteId S = net::SiteOf(Ticket);
		if (S == net::SiteId::RiverLine)
		{
			++L.Tickets[Ticket]; // a plain ticket (U9): no template moves, nothing governed
		}
		else
		{
			// A rival's ticket is a site promotion: the Value Governor pays it now or holds it on the Guest List, which never expires.
			const net::EventTemplate* T = net::ForEvent(Event).FindTemplate(Event);
			(void)site::play::Grant(L.SitePlay, L, S, site::play::Kind::Ticket, T ? T->Name : Ticket, FestTicketFace(Ticket), Ticket + "|" + Key,
				site::play::PlayDay(Now), Now);
		}
	}
	// (No next edition, or no satellite of the kind in it, e.g. Summer Slam's: the ticket closes with nothing to seat.)
	L.Sites.Done.insert(Key);
	L.Update2LinesRead |= life::U2Sites;
	world::FestTicketPaid(W, MarkKey);
	return true;
}

int FestStepCredits(life::State& L, const world::World& W, double Now)
{
	// §2.6.1 item 8: only a championship edition's step tickets (a ticket its satellites seat, its id "<series>-<step>"), after its crown,
	// once none of their runs is left (Network::NextFor), each once (festcredit:<ticket>); the Main ticket (year-free, weighed against
	// the brand's latest crowned edition) carries to the next edition and is never credited.
	int Credited = 0;
	for (auto It = L.Tickets.begin(); It != L.Tickets.end();)
	{
		const std::string Ticket = It->first;
		const size_t Dash = Ticket.rfind('-');
		// A rival's tickets only: RiverLine has no wallet, and RCOP's steps run every week (NextFor always places them).
		const bool Rival = It->second > 0 && Dash != std::string::npos && net::SiteOf(Ticket) != net::SiteId::RiverLine;
		const fest::Festival* F = Rival ? fest::FestivalOf(Ticket.substr(0, Dash)) : nullptr;
		F = F || !Rival ? F : FestMainTicketEdition(Ticket, W, Now);
		bool Seated = false;
		const int Step = F && fest::IsChampionship(F->Which) ? FestStepOf(*F, Ticket, Seated) : 0;
		const std::string Key = "festcredit:" + Ticket;
		const double Crown = Seated ? world::FestCrownedAt(W, F->SeriesId) : -1.0;
		net::EventInstance Next;
		const net::Network& N = net::ForEvent(Ticket);
		Chips* Wallet = Crown >= 0.0 && Now >= Crown && L.Sites.Done.count(Key) == 0 && !N.NextFor(Ticket, Now, Next) ? site::WalletOf(L.Sites, net::SiteOf(Ticket)) : nullptr;
		if (!Wallet)
		{
			++It;
			continue;
		}
		const Chips Cents = N.SeatValue(Ticket) * static_cast<Chips>(It->second);
		*Wallet += Cents;
		site::AddLine(L.Sites, net::SiteOf(Ticket), Now, 4, Cents,
			site::FillCopy(site::CopyText("ledger:festcredit"), {{"Festival", F->Name}, {"K", std::to_string(Step)}}));
		L.Sites.Done.insert(Key);
		L.Update2LinesRead |= life::U2Sites;
		It = L.Tickets.erase(It);
		++Credited;
	}
	return Credited;
}

std::pair<std::string, std::string> FestCrownLetter(const fest::Festival& F, double Points)
{
	const std::string Name = FestNameNoYear(F);
	const std::string Year = std::to_string(F.Year);
	return {site::FillCopy(site::CopyText("letter:fest:crown:subject"), {{"Festival", Name}, {"Year", Year}}),
		site::FillCopy(site::CopyText("letter:fest:crown"), {{"Festival", fest::NameAfterArticle(Name)}, {"Year", Year}, {"Points", Grouped(std::llround(Points))}})};
}
} // namespace session_fest_detail

std::optional<FestMoment> Session::PeekFestMoment() const
{
	// §2.8.5: the results screen's moment first (Kinds 1, 3 and 5, or a DEV one), then the belt's and the marks' (Kinds 2 and 4), which
	// wait until their done key is in the sites' book (a reload before the dismiss plays them again: they weren't seen).
	if (FestPending)
	{
		return FestPending;
	}
	return session_fest_detail::FestMarkMoment(LivingWorld, Life.Sites.Done);
}

void Session::DoneFestMoment()
{
	if (FestPending)
	{
		FestPending.reset(); // Kinds 1, 3 and 5 keep nothing, nor does a DEV moment (LEAD_CALLS R-7)
		return;
	}
	const std::optional<FestMoment> M = session_fest_detail::FestMarkMoment(LivingWorld, Life.Sites.Done);
	if (!M)
	{
		return;
	}
	Life.Sites.Done.insert((M->Kind == 2 ? "festbelt:" : "festmark:") + M->Key); // the session's site\tdone line (§1.7)
	Life.Update2LinesRead |= life::U2Sites;
	Save();
}

void Session::FestStep(double From, double To)
{
	// §2.9.3: every mark the hero keeps sends its line once, keyed, whenever a step sees it (CheckCalendar's after SitePlayStep, and
	// WorldSkip's after the advance), so a skip over the crown and a reload send each once. Never while the world's turn holds (§3.7
	// R6). The belt's T10 always (the cap's first slot), with the host's crown letter (L5-L6, routine mail that carries a seat, so it
	// goes even in a week the site has written; keyed with T10); FINALIST's and PODIUM's T14 only while the edition's win, title or
	// final-table line hasn't gone (its second). FW-6: then each unpaid crown ticket (festticket:<key>) and, after a championship's
	// crown, each step ticket no run is left for (festcredit:<ticket>), saved once if either moved.
	if (LivingWorld.Holding())
	{
		return;
	}
	std::vector<std::string> Unpaid; // the marks still holding a crown ticket (once an edition, so nearly always empty)
	for (const world::HeroFestMark& K : world::HeroMarks(LivingWorld))
	{
		if (K.Ticket && fest::kCrownTickets)
		{
			Unpaid.push_back(K.Key);
		}
		const bool Belt = K.Mark == world::FestMark::Title;
		if (!Belt && K.Mark != world::FestMark::Finalist && K.Mark != world::FestMark::Podium)
		{
			continue;
		}
		const std::string Key = (Belt ? "fest-belt-" : "fest-mark-") + K.Key;
		const fest::Festival* F = TextsSeen.count(Key) > 0 ? nullptr : fest::FestivalOf(K.Key);
		if (!F)
		{
			continue;
		}
		if (Belt)
		{
			// The letter first: T10's StoryText saves, and the key it writes keeps both from going twice.
			const std::pair<std::string, std::string> Letter = session_fest_detail::FestCrownLetter(*F, K.Points);
			site::PostLetter(Life, F->Site, WorldMinutes(), site::LetterRoutine, Letter.first, Letter.second);
			StoryText(Key, "Dee", fest::fest_text_detail::FestText("T10"));
		}
		else if (!session_fest_detail::FestWinLineSent(TextsSeen, K.Key))
		{
			const char* Grade = K.Mark == world::FestMark::Podium ? "PODIUM" : "FINALIST";
			StoryText(Key, "Dee",
				site::FillCopy(fest::fest_text_detail::FestText("T14"), {{"MARK", Grade}, {"Festival", F->Name}, {"Rank", Grouped(K.Rank)}, {"Scorers", Grouped(K.Scorers)}}));
		}
	}
	bool Moved = false;
	for (const std::string& Key : Unpaid)
	{
		for (const world::HeroFestMark& K : world::HeroMarks(LivingWorld))
		{
			if (K.Key == Key)
			{
				Moved = session_fest_detail::FestPayCrownTicket(Life, LivingWorld, K, WorldMinutes()) || Moved;
				break;
			}
		}
	}
	// The step refunds on each hour the step crosses (the crown's own clock: World::AdvanceTo's hourly slices; a skip crosses many).
	if (std::floor(To / 60.0) > std::floor(From / 60.0))
	{
		Moved = session_fest_detail::FestStepCredits(Life, LivingWorld, WorldMinutes()) > 0 || Moved;
	}
	if (Moved)
	{
		Save();
	}
}

namespace session_fest_detail
{
namespace
{
/** The ten brand keys as the console takes them. */
const char* const FestBrandKeysText = "rcop, ring, tco, slam, push, feign, writ, grit, pennant or reign";

/** The console's words, lower case. */
std::vector<std::string> FestWords(const std::string& Args)
{
	std::vector<std::string> Out;
	std::string Word;
	for (const char C : Args + " ")
	{
		if (C == ' ' || C == '\t')
		{
			if (!Word.empty())
			{
				Out.push_back(Word);
			}
			Word.clear();
		}
		else
		{
			Word += C >= 'A' && C <= 'Z' ? static_cast<char>(C - 'A' + 'a') : C;
		}
	}
	return Out;
}

/** "Oct 22 3:00 PM". */
std::string FestWhen(double Minute)
{
	return net::DateLabel(net::DayOf(Minute)) + " " + net::TimeLabel(Minute);
}

const char* FestPhaseName(fest::Phase P)
{
	switch (P)
	{
	case fest::Phase::Announced: return "Announced";
	case fest::Phase::Buildup: return "Buildup";
	case fest::Phase::Live: return "Live";
	case fest::Phase::MainDay: return "Main day";
	case fest::Phase::Closing: return "Closing";
	case fest::Phase::Afterglow: return "Afterglow";
	case fest::Phase::Off: break;
	}
	return "Off";
}

const char* FestMarkName(world::FestMark M)
{
	switch (M)
	{
	case world::FestMark::Top25: return "TOP 25%";
	case world::FestMark::Top10: return "TOP 10%";
	case world::FestMark::Finalist: return "FINALIST";
	case world::FestMark::Podium: return "PODIUM";
	case world::FestMark::Title: return "THE BELT";
	case world::FestMark::None:
	case world::FestMark::Count: break;
	}
	return "none";
}

/** A race row's name: the hero's "You", a person's screen name. */
std::string FestWho(const world::World& W, int Npc)
{
	const world::Npc* N = Npc >= 0 ? W.Get(Npc) : nullptr;
	return Npc < 0 ? std::string("You") : N ? N->Name : "#" + std::to_string(Npc);
}

/** The brand's latest edition already open by Today (its window holds Today, or it is over), else nullptr; First: its first edition. */
const fest::Festival* FestEditionNow(fest::Brand B, int Today, const fest::Festival*& First)
{
	const fest::Festival* Out = nullptr;
	First = nullptr;
	for (const fest::Festival& F : fest::All())
	{
		if (F.Which == B)
		{
			First = First ? First : &F;
			Out = F.FirstDay <= Today ? &F : Out;
		}
	}
	return Out;
}

/** The edition's next numbered event from Now (its events in start order, the Main among them; never a satellite): false when none
 *  is left. */
bool FestNextNumbered(const fest::Festival& F, double Now, net::EventInstance& Out)
{
	const net::Network& N = net::ForEvent(F.MainId);
	const double From = std::max(Now, static_cast<double>(F.FirstDay) * net::MinutesPerDay);
	for (const net::EventInstance& E : N.Window(From, static_cast<double>(F.LastDay + 1) * net::MinutesPerDay))
	{
		const net::EventTemplate& T = N.TemplateOf(E);
		if (T.Series == F.SeriesId && std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) == F.Satellites.end())
		{
			Out = E;
			return true;
		}
	}
	return false;
}

/** A title event's first prize in edition F (its run from the edition's first day; 0 when it has none). */
Chips FestFirstPrize(const fest::Festival& F, const std::string& TemplateId)
{
	const net::Network& N = net::ForEvent(TemplateId);
	net::EventInstance E;
	const double From = static_cast<double>(F.FirstDay) * net::MinutesPerDay;
	if (!N.NextFor(N.TicketOf(TemplateId), From, E) || E.Id.compare(0, TemplateId.size() + 1, TemplateId + "@") != 0)
	{
		return 0;
	}
	const std::vector<Chips> Pay = N.Payouts(E);
	return Pay.empty() ? 0 : Pay.front();
}

/** ss.Fest.Describe (§2.16). */
std::string FestDescribe(const world::World& W, const life::State& L, const rep::Public& P, double Now)
{
	const int Today = net::DayOf(Now);
	const int Year = world::YearOf(Today);
	std::string Out = "Festival Season, " + FestWhen(Now) + " (world day " + std::to_string(Today) + ")\n" + std::to_string(Year) + "'s windows:";
	for (const fest::Festival* F : fest::Festivals(world::YearStart(Year), world::YearStart(Year + 1) - 1))
	{
		Out += "\n  " + F->Name + " (" + net::Def(F->Site).Name + "): " + net::DateLabel(F->FirstDay) + " - " + net::DateLabel(F->LastDay) + ", Main " +
			FestWhen(F->MainStart) + ", " + FestPhaseName(fest::PhaseOf(*F, Now, &W));
	}
	const fest::Festival* Live = fest::FestivalOf(world::FestRaceKey(W));
	if (Live)
	{
		const std::pair<int, int> Me = world::FestStanding(W, -1);
		Out += "\nRace: " + Live->Name + ", " + Grouped(Me.second) + " scorers";
		int Rank = 0;
		for (const std::pair<int, world::FestScore>& R : world::FestTop(W, 10))
		{
			Out += "\n  " + std::to_string(++Rank) + ". " + FestWho(W, R.first) + " " + Grouped(std::llround(R.second.Points)) + " pts, " +
				std::to_string(R.second.Cashes) + (R.second.Cashes == 1 ? " cash" : " cashes");
		}
		Out += Me.first > 0 ? "\n  You: rank " + Grouped(Me.first) + " of " + Grouped(Me.second) : std::string("\n  You: not scoring yet");
		std::string Known;
		for (const std::pair<int, int>& K : world::FestKnown(W))
		{
			Known += (Known.empty() ? "" : ", ") + FestWho(W, K.first) + " (" + Grouped(K.second) + ")";
		}
		Out += "\n  People you know: " + (Known.empty() ? std::string("none scoring") : Known);
	}
	else
	{
		const fest::Festival* Next = fest::Next(Today);
		Out += "\nRace: none running" + (Next ? "; next " + Next->Name + " opens " + net::DateLabel(Next->FirstDay) : std::string());
	}
	// The last crown: the latest edition the world has crowned.
	std::string Crown = "none yet";
	const std::vector<fest::Festival>& All = fest::All();
	for (auto It = All.rbegin(); It != All.rend() && Crown == "none yet"; ++It)
	{
		const double At = It->FirstDay <= Today ? world::FestCrownedAt(W, It->SeriesId) : -1.0;
		for (const world::FestRollRow& R : At >= 0.0 ? world::FestRoll(W, It->Which) : std::vector<world::FestRollRow>())
		{
			if (R.Key == It->SeriesId && R.Crown)
			{
				Crown = It->Name + ": " + (R.Crown->Npc < 0 ? std::string("You") : R.Crown->Name) + ", " + Grouped(R.Crown->Entries) + " pts, " + FestWhen(At);
			}
		}
	}
	Out += "\nLast crown: " + Crown;
	std::string Pins;
	for (const world::HeroFestPin& Pin : world::HeroFestPins(W))
	{
		const fest::Festival* F = fest::FestivalOf(Pin.Key);
		Pins += std::string(Pins.empty() ? "" : "; ") + (F ? F->Name : Pin.Key) + " " + (Pin.Grade >= 3 ? "gold" : Pin.Grade == 2 ? "silver" : "outline") + " (" +
			std::to_string(Pin.Events) + " events, " + std::to_string(Pin.Cashes) + " cashes, " + std::to_string(Pin.Wins) + " wins)";
	}
	Out += "\nYour pins: " + (Pins.empty() ? std::string("none") : Pins);
	std::string Marks;
	for (const world::HeroFestMark& K : world::HeroMarks(W))
	{
		const fest::Festival* F = fest::FestivalOf(K.Key);
		const bool Paid = L.Sites.Done.count("festticket:" + K.Key) > 0;
		Marks += std::string(Marks.empty() ? "" : "; ") + (F ? F->Name : K.Key) + " " + FestMarkName(K.Mark) + ", " + Grouped(K.Rank) + " of " + Grouped(K.Scorers) + ", " +
			Grouped(std::llround(K.Points)) + " pts" + (K.Ticket ? ", crown ticket unpaid" : Paid ? ", crown ticket paid" : "");
	}
	Out += "\nYour marks: " + (Marks.empty() ? std::string("none") : Marks);
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const net::SiteDef& D = net::Def(S);
		const fest::Festival* F = D.Gate.FestivalEvents >= 0 ? fest::PassFestival(S, Now) : nullptr;
		Out += "\n" + D.Name + "'s Pass: rents " + std::to_string(L.RentsPaid) + " of " + std::to_string(std::max(2, D.Gate.MinRentsPaid)) + ", paid events " +
			std::to_string(std::max(P.OnlineEvents, P.Paid100)) +
			(F ? " of " + std::to_string(fest::PassNeeds(*F, D.Gate)) + ", " + F->Name + " in its window (" + FestPhaseName(fest::PhaseOf(*F, Now, &W)) + ")"
			   : std::string(", no festival in its window")) +
			(site::AccountOf(L.Sites, S) ? ", account open" : "");
	}
	return Out;
}

/** ss.Fest.Calendar [year] (§2.16). */
std::string FestCalendar(const world::World& W, int Year)
{
	std::string Out = std::to_string(Year) + "'s festival windows:";
	const std::vector<const fest::Festival*> Year1 = fest::Festivals(world::YearStart(Year), world::YearStart(Year + 1) - 1);
	for (const fest::Festival* F : Year1)
	{
		// The clear days: from the window's end to the site's next window.
		int Clear = -1;
		for (const fest::Festival& G : fest::All())
		{
			Clear = Clear < 0 && G.Site == F->Site && G.FirstDay > F->LastDay ? G.FirstDay - F->LastDay - 1 : Clear;
		}
		const double Crowned = world::FestCrownedAt(W, F->SeriesId);
		Out += "\n  " + F->Name + " (" + net::Def(F->Site).Name + "): " + net::DateLabel(F->FirstDay) + " - " + net::DateLabel(F->LastDay) + ", Main " +
			FestWhen(F->MainStart) + ", crown expected " + FestWhen(fest::LastEnd(*F)) + (Crowned >= 0.0 ? " (crowned " + FestWhen(Crowned) + ")" : std::string()) +
			(Clear >= 0 ? ", then " + std::to_string(Clear) + " clear days on " + net::Def(F->Site).Name : std::string());
	}
	return Year1.empty() ? "not yet: no festival in " + std::to_string(Year) : Out;
}

/** ss.Fest.Race [n] (§2.16). */
std::string FestRaceText(const world::World& W, int Count, int Today)
{
	const fest::Festival* Live = fest::FestivalOf(world::FestRaceKey(W));
	const std::vector<std::pair<int, world::FestScore>> Top = world::FestTop(W, Count);
	if (!Live && Top.empty())
	{
		const fest::Festival* Next = fest::Next(Today);
		return "No race is running" + (Next ? "; next: " + Next->Name + " opens " + net::DateLabel(Next->FirstDay) : std::string());
	}
	const std::pair<int, int> Me = world::FestStanding(W, -1);
	std::string Out = Live ? Live->Name + "'s race, " + Grouped(Me.second) + " scorers:" : std::string("The last race's final top:");
	int Rank = 0;
	for (const std::pair<int, world::FestScore>& R : Top)
	{
		Out += "\n  " + std::to_string(++Rank) + ". " + FestWho(W, R.first) + " " + Grouped(std::llround(R.second.Points)) + " pts, " + std::to_string(R.second.Cashes) +
			(R.second.Cashes == 1 ? " cash" : " cashes");
	}
	return Out + (Live ? (Me.first > 0 ? "\n  You: rank " + Grouped(Me.first) + " of " + Grouped(Me.second) : std::string("\n  You: not scoring yet")) : std::string());
}
} // namespace
} // namespace session_fest_detail

std::string Session::DebugFest(const std::string& Args)
{
	// §2.16's console hooks (game/Update4Commands.cpp passes "<verb> <args>", the verb in lower case): describe, calendar [year],
	// race [n]; hub <brand> [year] and look <brand|off> answer what they would open (the client's routing is the lead's edit, LEAD_CALLS
	// R-19); the dev verbs each say "(dev)": moment <brand> [belt|event|title|mark <grade>] (a Dev moment: numbers marked DEV, nothing
	// granted, no done key, R-7), pass and result <brand> cash|ft|win|s1 seat (through the world's result path). A brand's verb answers
	// "not yet" before its first edition opens.
	namespace sfd = session_fest_detail;
	if (fest::All().empty())
	{
		return "not yet: no festival exists in this build";
	}
	DrainWorld(); // the world's answer now (a held turn finishes first)
	const std::vector<std::string> A = sfd::FestWords(Args);
	const std::string Verb = A.empty() ? std::string() : A[0];
	const double FestNow = WorldMinutes();
	const int Today = net::DayOf(FestNow);
	const std::string Usage = "ss.Fest: describe, calendar [year], race [n], hub <brand> [year], look <brand|off>, moment <brand> [belt|event|title|mark "
							  "<top25|top10|finalist|podium>], pass, result <brand> <cash|ft|win|s1 seat>";
	if (Verb == "describe")
	{
		return sfd::FestDescribe(LivingWorld, Life, Record(), FestNow);
	}
	if (Verb == "calendar")
	{
		const int Year = A.size() > 1 ? std::atoi(A[1].c_str()) : world::YearOf(Today);
		return Year < fest::kFirstYear || Year > fest::kLastYear ? "calendar: a year from " + std::to_string(fest::kFirstYear) + " to " + std::to_string(fest::kLastYear)
																 : sfd::FestCalendar(LivingWorld, Year);
	}
	if (Verb == "race")
	{
		const int Count = A.size() > 1 ? std::max(1, std::min(100, std::atoi(A[1].c_str()))) : 10;
		return sfd::FestRaceText(LivingWorld, Count, Today);
	}
	if (Verb == "pass")
	{
		Life.RentsPaid = std::max(Life.RentsPaid, 2);
		LivingWorld.GrantHeroResults(20);
		Save();
		const rep::Public P = Record();
		return "pass: rents paid " + std::to_string(Life.RentsPaid) + ", 20 small events on the record (" + std::to_string(std::max(P.OnlineEvents, P.Paid100)) +
			" paid online events) (dev)";
	}
	if (Verb == "look" && A.size() > 1 && A[1] == "off")
	{
		return "look off: the site in front would drop its dev look (dev)";
	}
	if (Verb != "hub" && Verb != "look" && Verb != "moment" && Verb != "result")
	{
		return Usage;
	}
	const fest::Brand B = A.size() > 1 ? fest::BrandOf(A[1]) : fest::Brand::None;
	if (B == fest::Brand::None)
	{
		return Verb + ": a brand key: " + sfd::FestBrandKeysText;
	}
	const fest::Festival* First = nullptr;
	const fest::Festival* F = sfd::FestEditionNow(B, Today, First);
	if (!First)
	{
		return Verb + " " + A[1] + ": no edition";
	}
	if (Verb == "hub")
	{
		// That edition's hub (a year picks one; else the edition open now, or the next to come).
		const int Year = A.size() > 2 ? std::atoi(A[2].c_str()) : 0;
		const fest::Festival* H = nullptr;
		for (const fest::Festival& G : fest::All())
		{
			const bool Pick = G.Which == B && (Year > 0 ? G.Year == Year : G.LastDay + fest::kAfterglowDays >= Today);
			H = !H && Pick ? &G : H;
		}
		H = H ? H : Year > 0 ? nullptr : F;
		if (!H)
		{
			return "hub " + A[1] + ": no edition in " + std::to_string(Year);
		}
		const fest::Phase Ph = fest::PhaseOf(*H, FestNow, &LivingWorld);
		if (Ph == fest::Phase::Off && Today < H->FirstDay)
		{
			return "not yet: " + H->Name + "'s hub opens when it is announced (" + net::DateLabel(H->FirstDay - fest::kAnnounceDays) + ")";
		}
		const bool Rival = H->Site != net::SiteId::RiverLine;
		return "hub " + A[1] + ": would open " + H->Name + "'s hub on " + net::Def(H->Site).Name + " (" + sfd::FestPhaseName(Ph) + ")" +
			(Rival && !site::AccountOf(Life.Sites, H->Site) ? "; it needs an account on " + net::Def(H->Site).Name + " first" : std::string());
	}
	if (Verb == "look")
	{
		return "look " + A[1] + ": the site in front would wear " + (F ? F->Name : First->Name) + "'s look for screenshots, presentation only (dev)";
	}
	if (!F && (Verb == "moment" || Today < First->FirstDay - fest::kAnnounceDays))
	{
		return "not yet: " + First->Name + " opens " + fest::DateInSentence(First->FirstDay, Today);
	}
	if (Verb == "moment")
	{
		// A Dev moment (R-7): the UI plays it next (PeekFestMoment), numbers marked DEV; DoneFestMoment keeps no key for it.
		const std::string What = A.size() > 2 ? A[2] : std::string("main");
		FestMoment M;
		M.Dev = true;
		M.Which = F->Which;
		M.Key = F->SeriesId;
		M.Year = F->Year;
		if (What == "main")
		{
			M.Kind = 1;
			M.Prize = sfd::FestFirstPrize(*F, F->MainId);
		}
		else if (What == "event")
		{
			M.Kind = 3;
			net::EventInstance E;
			M.Prize = sfd::FestNextNumbered(*F, static_cast<double>(F->FirstDay) * net::MinutesPerDay, E) ? sfd::FestFirstPrize(*F, net::ForEvent(E.Id).TemplateOf(E).Id) : 0;
		}
		else if (What == "belt")
		{
			M.Kind = 2;
			M.Prize = 1000; // the race's points, DEV
			M.Mark = static_cast<int>(world::FestMark::Title);
			M.Rank = 1;
			M.Scorers = 500;
		}
		else if (What == "title")
		{
			if (!fest::IsChampionship(F->Which) || F->TitleIds.size() < 2)
			{
				return "moment " + A[1] + " title: " + F->Name + " has no Mini or Micro title (only a championship's)";
			}
			M.Kind = 5;
			M.Mark = 1; // the Mini
			M.Prize = sfd::FestFirstPrize(*F, F->TitleIds[1]);
		}
		else if (What == "mark")
		{
			const std::string Grade = A.size() > 3 ? A[3] : std::string();
			const world::FestMark Mark = Grade == "top25" ? world::FestMark::Top25 : Grade == "top10" ? world::FestMark::Top10 : Grade == "finalist" ? world::FestMark::Finalist
				: Grade == "podium" ? world::FestMark::Podium : world::FestMark::None;
			if (Mark == world::FestMark::None)
			{
				return "moment " + A[1] + " mark: a grade, top25, top10, finalist or podium";
			}
			M.Kind = 4;
			M.Mark = static_cast<int>(Mark);
			M.Prize = 600; // the race's points, DEV
			M.Rank = Mark == world::FestMark::Podium ? 2 : Mark == world::FestMark::Finalist ? 9 : Mark == world::FestMark::Top10 ? 40 : 110;
			M.Scorers = 500;
		}
		else
		{
			return "moment " + A[1] + ": belt, event, title or mark <top25|top10|finalist|podium> (none: the Main's)";
		}
		FestPending = M;
		return "moment " + A[1] + (A.size() > 2 ? " " + What + (What == "mark" ? " " + A[3] : std::string()) : std::string()) + ": " + F->Name +
			"'s moment plays next, its numbers marked DEV; it grants nothing (dev)";
	}
	// result <brand> cash|ft|win|s1 seat: one hero result in the live edition's next numbered event, through the world's result path.
	const std::string What = A.size() > 2 ? A[2] : std::string();
	const bool Seat = What == "s1" && A.size() > 3 && A[3] == "seat";
	if (What != "cash" && What != "ft" && What != "win" && !Seat)
	{
		return "result " + A[1] + ": cash, ft, win or s1 seat";
	}
	const fest::Festival* Live = nullptr;
	for (const fest::Festival& G : fest::All())
	{
		const bool Open = Seat ? G.FirstDay - fest::kAnnounceDays <= Today && Today <= G.MainDay : G.FirstDay <= Today && Today <= G.LastDay;
		Live = !Live && G.Which == B && Open ? &G : Live;
	}
	if (!Live)
	{
		return "not yet: no " + std::string(fest::BrandKey(B)) + " festival is live (" + (F ? F->Name + " has closed" : First->Name + " opens " + fest::DateInSentence(First->FirstDay, Today)) + ")";
	}
	const net::Network& N = net::ForEvent(Live->MainId);
	net::EventInstance E;
	if (Seat)
	{
		// A Step 1 run won (§2.16, v1.6): its seat, a Step 2 ticket, into the wallet; Dee's ladder line (FW-5's seat rule).
		const net::EventTemplate* Step1 = nullptr;
		for (const std::string& Id : Live->Satellites)
		{
			Step1 = !Step1 && Id.size() > 4 && Id.compare(Id.size() - 4, 4, "-s1a") == 0 ? N.FindTemplate(Id) : Step1;
		}
		if (!Step1 || Step1->SeatTicket.empty())
		{
			return "result " + A[1] + " s1 seat: " + Live->Name + " has no Step 1 (only a championship's ladder)";
		}
		if (!N.NextFor(N.TicketOf(Step1->Id), FestNow, E))
		{
			return "result " + A[1] + " s1 seat: no Step 1 run is left in " + Live->Name;
		}
		LivingWorld.HeroEntered(E.Id, N.NameOf(E));
		LivingWorld.HeroFinished(E.Id, 1, 0, world::World::TableReport());
		++Life.Tickets[Step1->SeatTicket];
		const std::string Line = sfd::FestSeatText(E.Id, Step1->SeatTicket, Step1->BuyInCents);
		StoryText("seat-" + Step1->SeatTicket, "Dee", Line.empty() ? std::string(sfd::FestLadderLine()) : Line);
		Save();
		return "result " + A[1] + " s1 seat: " + N.NameOf(E) + " (" + sfd::FestWhen(E.Start) + ") won, a ticket for " + Step1->SeatTicket + " in the wallet (dev)";
	}
	if (!sfd::FestNextNumbered(*Live, FestNow, E))
	{
		return "result " + A[1] + " " + What + ": no numbered event is left in " + Live->Name;
	}
	const std::vector<Chips> Pay = N.Payouts(E);
	const int Paid = static_cast<int>(Pay.size());
	const int Place = std::max(1, std::min(Paid, What == "cash" ? Paid : What == "ft" ? 5 : 1));
	const Chips Prize = Paid > 0 ? Pay[static_cast<size_t>(Place - 1)] : 0;
	LivingWorld.HeroEntered(E.Id, N.NameOf(E));
	LivingWorld.HeroFinished(E.Id, Place, Prize, world::World::TableReport());
	Save();
	const std::pair<int, int> Me = world::FestStanding(LivingWorld, -1);
	return "result " + A[1] + " " + What + ": " + N.NameOf(E) + " (" + sfd::FestWhen(E.Start) + "), " + Ordinal(Place) + " of " + Grouped(E.Entries) + " for " + Money(Prize) +
		"; race rank " + (Me.first > 0 ? Grouped(Me.first) + " of " + Grouped(Me.second) : std::string("none")) + " (dev)";
}
} // namespace ss
