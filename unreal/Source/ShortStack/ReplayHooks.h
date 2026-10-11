// Scripted, repeatable nights for comparing two builds line for line (next_segment_spec 8.2: Phase A ends when a Nightly
// from the desk to the cage and a Dee's night replay the same ledger and captions as the build before it). The driver is
// ReplayHooks.cpp's console commands (ss.Replay.*); the table calls the two hooks below. The same file goes into the
// earlier build as a patch (E:/ShortStack/durable/segment/tools/replay), so both play and record alike. Testing only:
// nothing here runs unless a replay is under way.
#pragma once

#include "CoreMinimal.h"

namespace ReplayHooks
{
/** The seed the card table deals from while a replay runs (0 otherwise: the table seeds from the clock, as ever). */
uint32 Seed();
/** A line the table hands its HUD (ABackRoomTable::TakeLines), recorded while a replay runs. bRead: a whisper. */
void Caption(const FString& Speaker, const FString& Text, bool bRead);
} // namespace ReplayHooks
