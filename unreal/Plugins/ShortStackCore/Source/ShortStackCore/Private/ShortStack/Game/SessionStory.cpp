// Update 4, the session's side of the stories (SPEC v1.7 §3.15 R6, §3.16, §3.19): the StoryNotes drained into texts and Mail, the
// follows, the Notebook's view, NoteReach's calls and the monthly rankings' letter. W-story's file from G2 (the Notebook's and the
// STORY tab's switches are W-ui's reads of it).
//
// Phase A (§6 PA-19, PA-38, PA-47, PA-50; the lead): stubs that keep today's behaviour. WorldStep calls DrainStoryNotes after its
// news loop and the world keeps no notes, so nothing is sent; CheckCalendar hands World::NoteReach the follower count once a world
// day (a no-op until G2). The Notebook's icon and the card's STORY tab stay hidden, and ss.Story.* and ss.Tpi.* answer that they
// aren't built yet.
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

#include <string>

namespace ss
{
void Session::DrainStoryNotes()
{
	// Phase A: World::TakeStoryNotes hands over nothing (G2, W-story: each note through StoryText, keyed once, or Mail's letter with
	// Source "tpi", §3.15 R6).
}

bool Session::NotebookShown() const
{
	return false;
}

bool Session::StoryTabShown() const
{
	return false;
}

std::string Session::DebugStory(const std::string& /*Args*/)
{
	return "Not built yet: the stories and the TPI come with Update 4's G2.";
}
} // namespace ss
