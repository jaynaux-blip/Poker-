// The riverline_ui suite's kit for the next segment's per-track files (next_segment_spec §7.6). Each track's additions
// (its draw-list goldens, ControlParity, SettingsMigration...) live in its own tests/ui_seg_<track>.cpp, compiled into
// ui_test and called from main() in ui_main.cpp after the suite's own checks, so no two tracks edit one test file.
//
// In a track's file: the checks go in its entry point below; helpers go in a namespace of its own inside ui_test
// (ui_test::badges_seg, ...), so two files never define the same name. Expect counts a failure. TableMeasurer measures
// text with the Chromium metrics (Roboto) the goldens use; WriteList saves a draw list as <OutDir>/<Name>.json when
// ui_test is given an out-dir (web/scripts/render-drawlists.mjs replays them); Emit draws a RiverLine page and saves it.
#pragma once

#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/RiverLine.h"
#include "TestFontMetrics.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ui_test
{
extern int Failures;
extern std::string OutDir;
void Expect(bool Condition, const char* What);

/** Text widths from the Chromium metrics table (Roboto, the family Unreal's UI font uses). */
struct TableMeasurer : ss::ui::TextMeasurer
{
	static const float* Ascii(ss::ui::Font F)
	{
		switch (F)
		{
		case ss::ui::Font::Bold: return test_font::BoldAscii;
		case ss::ui::Font::Mono: return test_font::MonoAscii;
		case ss::ui::Font::Black: return test_font::BlackAscii;
		case ss::ui::Font::Light: return test_font::LightAscii;
		default: return test_font::RegularAscii;
		}
	}
	static const float* Special(ss::ui::Font F)
	{
		switch (F)
		{
		case ss::ui::Font::Bold: return test_font::BoldSpecial;
		case ss::ui::Font::Mono: return test_font::MonoSpecial;
		case ss::ui::Font::Black: return test_font::BlackSpecial;
		case ss::ui::Font::Light: return test_font::LightSpecial;
		default: return test_font::RegularSpecial;
		}
	}
	float Width(const std::string& Text, ss::ui::Font Face, float SizePx) const override
	{
		float Sum = 0.0f;
		for (size_t I = 0; I < Text.size();)
		{
			const unsigned char C0 = static_cast<unsigned char>(Text[I]);
			uint32_t Cp = C0;
			size_t Len = 1;
			if (C0 >= 0xF0) { Len = 4; Cp = C0 & 0x07u; }
			else if (C0 >= 0xE0) { Len = 3; Cp = C0 & 0x0Fu; }
			else if (C0 >= 0xC0) { Len = 2; Cp = C0 & 0x1Fu; }
			for (size_t K = 1; K < Len && I + K < Text.size(); ++K)
			{
				Cp = (Cp << 6) | (static_cast<unsigned char>(Text[I + K]) & 0x3Fu);
			}
			I += Len;
			float Wd = 55.0f;
			if (Cp >= 32 && Cp < 127) { Wd = Ascii(Face)[Cp - 32]; }
			else if (Cp == 0xB7) { Wd = Special(Face)[0]; }
			else if (Cp == 0x2014) { Wd = Special(Face)[1]; }
			else if (Cp == 0x2013) { Wd = Special(Face)[2]; }
			else if (Cp == 0x2026) { Wd = Special(Face)[3]; }
			else if (Cp == 0xE9) { Wd = Special(Face)[4]; }
			Sum += Wd;
		}
		return Sum * SizePx / 100.0f;
	}
	float Ascent(ss::ui::Font Face, float SizePx) const override
	{
		const float A = Face == ss::ui::Font::Bold ? test_font::BoldAscent : Face == ss::ui::Font::Mono ? test_font::MonoAscent : Face == ss::ui::Font::Black ? test_font::BlackAscent : Face == ss::ui::Font::Light ? test_font::LightAscent : test_font::RegularAscent;
		return A * SizePx / 100.0f;
	}
};

struct QuietHooks : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Texts;
	void Text(const std::string& From, const std::string& Body) override { Texts.push_back({From, Body}); }
};

/** The baseline of the first text run in a frame that contains Needle, or -1. */
float TextAt(const ss::ui::DrawList& L, const std::string& Needle);
bool Drew(const ss::ui::DrawList& L, const std::string& Needle);
/** A text run that is exactly Text. */
bool DrewRun(const ss::ui::DrawList& L, const std::string& Text);
/** Draws RL's current page (RiverLine::Width x Height), checks it drew, and saves it as Name. */
void Emit(const std::string& Name, ss::ui::RiverLine& RL, double Now);
/** Saves a draw list as <OutDir>/<Name>.json (nothing without an out-dir). */
void WriteList(const std::string& Name, const ss::ui::DrawList& L);
/** Runs the session for some real seconds. */
double Step(ss::Session& S, double Now, double Seconds);

// The tracks' entry points, called from main() in this order.
void SegStake();   // core-stake: tests/ui_seg_stake.cpp
void SegSponsor(); // core-sponsor: tests/ui_seg_sponsor.cpp
void SegTableUi(); // core-tableui: tests/ui_seg_tableui.cpp
void SegBadges();  // core-badges: tests/ui_seg_badges.cpp
void SegLaptop();  // ui-laptop: tests/ui_seg_laptop.cpp
} // namespace ui_test
