// Update 4, the session's side of Festival Season (SPEC v1.7 §2.8.5, §2.9): the champion's, the belt's, a mark's and a title's
// moments, FestStep's texts, letters, crown tickets and refunds, Dee's lines and Rerun's titles (fest::TextInventory) and ss.Fest.*.
// F-world's file from G1.
//
// Phase A (§6 PA-37; the lead): stubs that keep today's behaviour. No festival exists (fest::All() is empty), so no moment waits,
// FestStep (CheckCalendar's, after SitePlayStep) has nothing to send and ss.Fest.* answers that it isn't built yet.
// G1's FW-1 (F-world): the festival texts' one table and fest::TextInventory over it (below); the moments, FestStep and ss.Fest.* are
// still Phase A's stubs.
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Session.h"

#include <optional>
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

std::optional<FestMoment> Session::PeekFestMoment() const
{
	return std::nullopt;
}

void Session::DoneFestMoment()
{
}

void Session::FestStep(double /*From*/, double /*To*/)
{
}

std::string Session::DebugFest(const std::string& /*Args*/)
{
	return "Not built yet: Festival Season comes with Update 4's G1.";
}
} // namespace ss
