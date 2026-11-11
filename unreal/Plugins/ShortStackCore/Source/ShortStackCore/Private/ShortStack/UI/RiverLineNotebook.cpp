// Update 4, the Notebook (SPEC v1.7 §3.19.1): the hero's tenth laptop app, its seven tabs (FOLLOWING, SINCE YOU LAST LOOKED, NAMES TO
// WATCH, RISERS, FIRST-YEARS, THE SCENE, YOU) and ss.Laptop.Open notebook [tab]. W-ui's file from G2 (S-ui adds the crowd's rows at
// G4).
//
// Phase A (§6 PA-47, PA-50; the lead): stubs. The taskbar never shows the app (Session::NotebookShown is false), so NotebookApp is
// never reached, and NotebookOpen opens nothing (ss.Laptop.Open answers that nothing has that name in this build).
#include "ShortStack/UI/RiverLine.h"

#include <string>

namespace ss
{
namespace ui
{
void RiverLine::NotebookApp(double /*Now*/)
{
}

bool RiverLine::NotebookOpen(const std::string& /*Tab*/, double /*Now*/)
{
	return false;
}
} // namespace ui
} // namespace ss
