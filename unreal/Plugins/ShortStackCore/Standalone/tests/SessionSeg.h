// The night_one_session suite's kit for the next segment's per-track files (next_segment_spec §7.6). Each track's
// additions live in its own tests/session_seg_<track>.cpp, compiled into session_test and called from main() in
// session_main.cpp after the suite's own checks, so no two tracks edit one test file.
//
// In a track's file: the checks go in its entry point below; helpers go in a namespace of its own inside session_test
// (session_test::stake_seg, ...), so two files never define the same name. Expect counts a failure. Hooks is the
// suite's SessionHooks (the last save in Last, the phone's texts in Inbox and Got); PlayOut, Wait and Walk drive a
// session as the suite's own checks do.
#pragma once

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace session_test
{
extern int Failures;
void Expect(bool Condition, const char* What);

struct Hooks : ss::SessionHooks
{
	std::map<ss::SoundId, int> Sounds;
	int Texts = 0;
	int Cans = 0;
	int Beats = 0;
	int Saves = 0;
	ss::SaveData Last;
	std::vector<std::pair<std::string, std::string>> Inbox;
	void Sound(ss::SoundId Id, double) override { ++Sounds[Id]; }
	void Text(const std::string& From, const std::string& Body) override
	{
		++Texts;
		Inbox.emplace_back(From, Body);
		std::printf("  [phone] %s: %s\n", From.c_str(), Body.c_str());
	}
	/** A text from From with Part in it, since the inbox held Since. */
	bool Got(size_t Since, const char* From, const char* Part) const
	{
		for (size_t I = Since; I < Inbox.size(); ++I)
		{
			if (Inbox[I].first == From && Inbox[I].second.find(Part) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}
	void Heartbeat(bool On) override { Beats += On ? 1 : 0; }
	void AddCan() override { ++Cans; }
	int OnAirs = 0;
	int GearEvents = 0;
	void OnAir(bool On) override { OnAirs += On ? 1 : 0; }
	void GearChanged(const std::string&, bool) override { ++GearEvents; }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

/** Runs until the results screen; the hero calls, raises or folds at random. */
double PlayOut(ss::Session& S, ss::Rng& Choice, double Now, int& Decisions);
/** Runs the session for some real seconds (the lobby clock, time skips and the calendar move). */
double Wait(ss::Session& S, double Now, double Seconds);
/** Walks the session's clock on a minute at a time (the phone's moments are crossed as they come). */
double Walk(ss::Session& S, double Now, double ToWorld);

// The lead's Phase A checks (the save fixtures, the live record, the entry lines, funding, Rounds, the settings), then
// the tracks' entry points, called from main() in this order.
void SegLead();    // the lead: tests/session_seams.cpp
void SegStake();   // core-stake: tests/session_seg_stake.cpp
void SegSponsor(); // core-sponsor: tests/session_seg_sponsor.cpp
void SegTableUi(); // core-tableui: tests/session_seg_tableui.cpp
// Update 2's (update 2 SPEC §6.7), after the segment's: the lead's carried checks first (session_main.cpp: EconomyPacing,
// MigrationAll), then the tracks'.
void SegAuto();  // core-autoplay: tests/session_seg_auto.cpp
void SegVideo(); // core-video: tests/session_seg_video.cpp
void SegSites(); // core-sites: tests/session_seg_sites.cpp
void SegEcon();  // core-econ: tests/session_seg_econ.cpp
} // namespace session_test
