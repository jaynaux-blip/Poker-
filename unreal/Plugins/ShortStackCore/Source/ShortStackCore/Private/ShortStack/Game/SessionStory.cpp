// Update 4, the session's side of the stories (SPEC v1.7 §3.15 R6, §3.16, §3.19): the StoryNotes drained into texts and Mail, the
// follows, the Notebook's view, NoteReach's calls and the monthly rankings' letter. W-story's file from G2.
//
// Phase A (§6 PA-19; the lead): created empty. The session's hook calls into it (DrainStoryNotes from WorldStep, NoteReach from
// CheckCalendar, §6 PA-38) land with the session's group (a1-rest), which declares them in H/Session.h.
#include "ShortStack/Game/World.h"
