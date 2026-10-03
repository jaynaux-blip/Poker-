// The career save: everything ss::SaveData holds (bankroll, history, the living world), on disk.
//
// The living world makes the save a few megabytes of text, so it is stored compressed (Oodle) as UTF-8 in its own
// file, Saved/SaveGames/NightOne.ssave, written whole to a temporary file and then moved over the old one (a crash
// mid-write leaves the last good save). A career from before this format (the "NightOne" save slot) still loads, and
// is kept as NightOne.sav.migrated once the new file exists.
//
// Night One saves often (registrations, results, purchases): FCareerSaver does the expensive part of each save (the
// world's text, compression, the write) on a worker thread, one save at a time, the newest one waiting its turn.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Session.h"
#include "Tasks/Task.h"

#include <memory>
#include <string>

namespace CareerSave
{
/** The saved career's text (empty and false when there's none). */
bool LoadText(std::string& OutText);
/** Whether a career has been saved (in either format). */
bool Exists();
/**
 * Writes the career now, on the calling thread. bQuick packs it faster and a little larger (for a save on the game
 * thread as a scene ends: a few milliseconds instead of tens).
 */
bool SaveNow(const std::string& Text, bool bQuick = true);
/** Serializes a save, writing a frozen world's text into it first when it came without one. */
std::string Serialize(ss::SaveData& Data);
} // namespace CareerSave

/** Writes saves on a worker thread, in order, never two at once; the latest save waits while one is written. */
class FCareerSaver
{
public:
	~FCareerSaver();
	/** Game thread: save this (now, or as soon as the save being written is done). */
	void Submit(const ss::SaveData& Data);
	/** Game thread, every frame: starts the waiting save once the worker is free. */
	void Tick();
	/** Game thread: finishes the save being written and writes the waiting one, before the scene ends. */
	void Flush();
	bool IsBusy() const { return InFlight.IsValid() && !InFlight.IsCompleted(); }

private:
	void Start(ss::SaveData&& Data);
	/** Worker (or the game thread in Flush, with no worker running): the whole save. */
	void Write(ss::SaveData& Data);

	UE::Tasks::FTask InFlight;
	TOptional<ss::SaveData> Waiting;
	// The world's text for the last snapshot written (snapshots repeat until the world moves on).
	std::shared_ptr<const ss::world::World> TextOf;
	std::string WorldText;
};
