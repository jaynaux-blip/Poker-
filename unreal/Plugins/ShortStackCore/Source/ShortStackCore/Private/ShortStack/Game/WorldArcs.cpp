// Update 4, the Arc Book (SPEC v1.7 §3.14): opening, beats, closing and the caps (World::Arcs, H/WorldStory.h's ArcBook). W-story's
// file from G2.
//
// Phase A (§6 PA-19; the lead): no arc opens; the book stays empty, so world\tarc is never written. The file holds no body yet: its
// entry points (ArcWeek's ranges, the beat hooks) are G2's, reached through story::Access and the TurnUnit list (H/World.h).
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"
