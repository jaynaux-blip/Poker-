// UiSeg.h's kit for update 3's two UI suites (update 3 SPEC §7.7): rivals_ui (tests/ui_seg_ignite.cpp, ui_seg_rit.cpp) and
// rerun_ui (tests/ui_seg_video3.cpp, ui_seg_thumbs.cpp) run apart from riverline_ui, which already runs 64 s of its 75 s
// budget, so one broken seg never blocks every UI track. The same kit riverline_ui's ui_main.cpp defines (Expect, the
// draw-list readers, Emit, WriteList, Step), for their mains (rivals_ui_main.cpp, rerun_ui_main.cpp). The lead's file.
#include "UiSeg.h"

#include "ShortStack/UI/RiverLine.h"

#include <cstdio>
#include <string>

namespace ui_test
{
int Failures = 0;
std::string OutDir;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

float TextAt(const ss::ui::DrawList& L, const std::string& Needle)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Needle) != std::string::npos)
		{
			return Cmd.Text.BaselineY;
		}
	}
	return -1.0f;
}

bool Drew(const ss::ui::DrawList& L, const std::string& Needle)
{
	return TextAt(L, Needle) >= 0.0f;
}

bool DrewRun(const ss::ui::DrawList& L, const std::string& Text)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			return true;
		}
	}
	return false;
}

void WriteList(const std::string& Name, const ss::ui::DrawList& L)
{
	if (!OutDir.empty())
	{
		if (FILE* F = std::fopen((OutDir + "/" + Name + ".json").c_str(), "wb"))
		{
			const std::string J = L.ToJson();
			std::fwrite(J.data(), 1, J.size(), F);
			std::fclose(F);
		}
	}
}

void Emit(const std::string& Name, ss::ui::RiverLine& RL, double Now)
{
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
	RL.Draw(C, Now);
	Expect(!L.Cmds.empty() && L.Vertices.size() > 100, "frame drew something");
	WriteList(Name, L);
}

double Step(ss::Session& S, double Now, double Seconds)
{
	const double End = Now + Seconds;
	while (Now < End)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}
} // namespace ui_test
