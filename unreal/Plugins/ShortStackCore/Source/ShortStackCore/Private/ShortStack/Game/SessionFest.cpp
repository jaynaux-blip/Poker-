// Update 4, the session's side of Festival Season (SPEC v1.7 §2.8.5, §2.9): the champion's, the belt's, a mark's and a title's
// moments, FestStep's texts, letters, crown tickets and refunds, Dee's lines and Rerun's titles (fest::TextInventory) and ss.Fest.*.
// F-world's file from G1.
//
// Phase A (§6 PA-37; the lead): stubs that keep today's behaviour. No festival exists (fest::All() is empty), so no moment waits,
// FestStep (CheckCalendar's, after SitePlayStep) has nothing to send and ss.Fest.* answers that it isn't built yet.
// G1's FW-1 (F-world): the festival texts' one table and fest::TextInventory over it (below). FW-5: the session's texts and moments
// (SessionFestDetail.h: ShowResults' festival branch and the seat line, T12's fill, the cap, the marks' moments, Rerun's festival
// stories), PeekFestMoment, DoneFestMoment and FestStep's belt and mark texts; FestStep's crown letter, tickets and refunds and
// ss.Fest.* are FW-6's (DebugFest still Phase A's stub).
#include "SessionFestDetail.h"
#include "SitesDetail.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
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
	Out.emplace_back("Festival", fest::NameAfterArticle(F->Name)); // V1, V4, V6-V9 write "the {Festival}"; V3 and V5 read it the same
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

void Session::FestStep(double /*From*/, double /*To*/)
{
	// §2.9.3: every mark the hero keeps sends its line once, keyed, whenever a step sees it (CheckCalendar's after SitePlayStep, and
	// WorldSkip's after the advance), so a skip over the crown and a reload send each once. Never while the world's turn holds (§3.7
	// R6). The belt's T10 always (the cap's first slot); FINALIST's and PODIUM's T14 only while the edition's win, title or final-table
	// line hasn't gone (its second). The crown letter, the tickets and the refunds are FW-6's.
	if (LivingWorld.Holding())
	{
		return;
	}
	for (const world::HeroFestMark& K : world::HeroMarks(LivingWorld))
	{
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
			StoryText(Key, "Dee", fest::fest_text_detail::FestText("T10"));
		}
		else if (!session_fest_detail::FestWinLineSent(TextsSeen, K.Key))
		{
			const char* Grade = K.Mark == world::FestMark::Podium ? "PODIUM" : "FINALIST";
			StoryText(Key, "Dee",
				site::FillCopy(fest::fest_text_detail::FestText("T14"), {{"MARK", Grade}, {"Festival", F->Name}, {"Rank", Grouped(K.Rank)}, {"Scorers", Grouped(K.Scorers)}}));
		}
	}
}

std::string Session::DebugFest(const std::string& /*Args*/)
{
	return "Not built yet: Festival Season comes with Update 4's G1.";
}
} // namespace ss
