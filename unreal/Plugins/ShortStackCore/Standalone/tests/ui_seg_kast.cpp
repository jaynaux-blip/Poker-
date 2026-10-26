// kast_ui: Kast's screens under the creator update (SPEC_CREATORS §5.3, §9.1, §9.3; ui-kast's file). Every count draws
// View::Count / ShownCount and they agree on every surface (the pill, StreamSide, the VIEWERS tile, the directory card, the 24-inch
// Studio, the tile strip's reserve); the Chat tab carries no number and "N watching · M chatting" is never inverted; Heat's
// thermometer (never a flame), the ghost fill and the why lines; Event Mode's bar and gold pill; StreamSide's one action slot,
// the waiting chips and one toast a real minute; Hold or Crack's counts and the equity marker; the name menu with Nod; the
// peak-first end card with empty blocks collapsed; the forecast; the rank; Browse's chips and shelves; the Community page's
// Heat, Ping, raids, week, Stamina and payouts; the stream preview draws stills; the copy lint and both trademark tiers over
// rlkast's strings and every Kast page drawn here; the Kast pages' frame budgets (<= 1.5 ms warm, <= 2.0 ms at a final table
// with the Broadcast and Hold or Crack open). States go up through Stream::DebugView (nothing ticks between it and the draw).
// Compiled into kast_ui_test, called from kast_ui_main.cpp (the kit is UiSeg.h; its definitions are ui_seg_kit.cpp).
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastCopy.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineKastGrowth.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SecondScreen.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
#if defined(_WIN32)
#include <intrin.h>
// The thread's own cycles (kernel32), declared here rather than through <windows.h> and its macros.
extern "C" __declspec(dllimport) int __stdcall QueryThreadCycleTime(void* ThreadHandle, unsigned long long* CycleTime);
extern "C" __declspec(dllimport) void* __stdcall GetCurrentThread();
#endif

namespace ss
{
namespace ui
{
namespace rlkast_detail
{
// RiverLineKastGrowth.cpp's (not in the header): Event Mode's prestige diamonds, one to four by P.
int KwDiamonds(double P);
} // namespace rlkast_detail
} // namespace ui
} // namespace ss

namespace ui_test
{
namespace kast_seg
{
using ss::ui::DrawList;
using ss::ui::Rect;
namespace kast = ss::kast;
namespace copy = ss::kast::copy;
namespace kw = ss::ui::rlkast_detail;

int Checks = 0;

void Check(bool Condition, const char* What)
{
	++Checks;
	Expect(Condition, What);
}

/** A laptop, its session and the clock, with a stream kit. */
struct KDesk
{
	QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit KDesk(const char* Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		S.BankrollCents = 400000;
		RL.UI.Ptr.Active = false;
		for (const std::string& Id : {ss::gear::FirstPcUpgrade().Id, std::string("webcam-1080"), std::string("overlay-pack"), std::string("monitor-24")})
		{
			S.Buy(Id);
		}
		// A career's channel is under the new model (NewCareer, or its first stream under this build, sets U3Kast): a desk that
		// fakes a channel with streams behind it is that, not an old save before its first stream here (whose Studio previews
		// the go-live's seeding: L8, checked in Heat).
		S.Life.Update3LinesRead |= ss::life::U3Kast;
	}
	bool Live()
	{
		return S.GoLive().empty() && S.Streaming();
	}
	DrawList Frame()
	{
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	DrawList Shot(const std::string& Name)
	{
		DrawList L = Frame();
		WriteList("kast_ui_" + Name, L);
		return L;
	}
	void Away()
	{
		RL.UI.Ptr.Active = false;
		RL.UI.Ptr.X = -1000.0f;
		RL.UI.Ptr.Y = -1000.0f;
		RL.UI.Ptr.Down = false;
	}
	void Click(float X, float Y)
	{
		auto Fr = [&](bool Down, bool Pressed, bool Released) {
			RL.UI.Ptr.Active = true;
			RL.UI.Ptr.X = X;
			RL.UI.Ptr.Y = Y;
			RL.UI.Ptr.Down = Down;
			RL.UI.Ptr.Pressed = Pressed;
			RL.UI.Ptr.Released = Released;
			Frame();
		};
		Fr(true, true, false);
		Now += 0.05;
		Fr(false, false, true);
		Now += 0.05;
		Away();
	}
	void Studio()
	{
		RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
		RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	}
};

struct KRun
{
	bool Found = false;
	float X = 0.0f, Base = 0.0f, W = 0.0f, Size = 0.0f;
	ss::ui::Color Col;
};

/** The first text run that is Text (Has: contains it) with its baseline inside Area (W 0: anywhere). */
KRun RunIn(const DrawList& L, const std::string& Text, const TableMeasurer& M, const Rect& Area = {0.0f, 0.0f, 0.0f, 0.0f}, bool Has = false)
{
	KRun Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const ss::ui::TextItem& T = Cmd.Text;
		if (Area.W > 0.0f && (T.X + 1.0f < Area.X || T.X > Area.X + Area.W || T.BaselineY < Area.Y || T.BaselineY > Area.Y + Area.H))
		{
			continue;
		}
		if (Has ? T.Text.find(Text) != std::string::npos : T.Text == Text)
		{
			Out.Found = true;
			Out.X = T.X;
			Out.Base = T.BaselineY;
			Out.Size = T.SizePx;
			Out.W = M.Width(T.Text, T.Face, T.SizePx);
			Out.Col = T.Col;
			return Out;
		}
	}
	return Out;
}

/** Prints the runs inside Area (a failed check's context). */
void DumpIn(const DrawList& L, const Rect& Area, const char* What)
{
	std::printf("  %s, the runs in (%.0f, %.0f, %.0f, %.0f):\n", What, Area.X, Area.Y, Area.W, Area.H);
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const ss::ui::TextItem& T = Cmd.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && T.X + 1.0f >= Area.X && T.X <= Area.X + Area.W && T.BaselineY >= Area.Y && T.BaselineY <= Area.Y + Area.H)
		{
			std::printf("    \"%s\" at %.1f, %.1f size %.1f\n", T.Text.c_str(), T.X, T.BaselineY, T.SizePx);
		}
	}
}

void ClickRun(KDesk& D, const KRun& R)
{
	if (R.Found)
	{
		D.Click(R.X + R.W / 2.0f, R.Base - R.Size * 0.35f);
	}
}

bool SameColour(const ss::ui::Color& A, const ss::ui::Color& B)
{
	return std::fabs(A.R - B.R) < 0.01f && std::fabs(A.G - B.G) < 0.01f && std::fabs(A.B - B.B) < 0.01f;
}

/** Every text run drawn (for the trademark, U1 and flame screens over the pages). */
std::vector<std::string> AllTexts;

void Keep(const DrawList& L)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			AllTexts.push_back(Cmd.Text.Text);
		}
	}
}

/** Text runs whose box leaves Region (a card, the page). */
int Overflows(const DrawList& L, const Rect& Region, const TableMeasurer& M, const char* What)
{
	int Bad = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const ss::ui::TextItem& T = Cmd.Text;
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || T.X + 1.0f < Region.X || T.X > Region.X + Region.W || T.BaselineY < Region.Y || T.BaselineY > Region.Y + Region.H)
		{
			continue;
		}
		const float W = M.Width(T.Text, T.Face, T.SizePx);
		if (T.X + W > Region.X + Region.W + 0.5f)
		{
			++Bad;
			std::printf("  %s: \"%s\" runs to %.1f past %.1f\n", What, T.Text.c_str(), T.X + W, Region.X + Region.W);
		}
	}
	return Bad;
}

/** The thread's own CPU time in ms (its cycles at the TSC's rate), so other builds on the machine don't count against a page. */
double KastCpuMs()
{
#if defined(_WIN32)
	static const double TscPerMs = []() {
		const unsigned long long C0 = __rdtsc();
		const auto T0 = std::chrono::steady_clock::now();
		while (std::chrono::steady_clock::now() - T0 < std::chrono::milliseconds(40))
		{
		}
		const unsigned long long C1 = __rdtsc();
		return static_cast<double>(C1 - C0) / std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	}();
	unsigned long long Cycles = 0;
	QueryThreadCycleTime(GetCurrentThread(), &Cycles);
	return static_cast<double>(Cycles) / TscPerMs;
#else
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

struct KBench
{
	std::string Name;
	double Wall = 0.0;
	double Cpu = 0.0;      // warm: the laptop's clock stepping 1/30 s a draw, as the game redraws it
	double Capture = 0.0;  // every draw at one instant, so every draw re-captures the stream preview's client
	double Max = 0.0;      // the worst warm draw: each batch's worst, the best batch's (a re-capturing draw, on a page with the preview)
	double MaxAll = 0.0;   // the worst of all 50 warm draws (whatever else the machine did in that one)
	size_t Vertices = 0;
	long long Copies = 0;  // the warm draws that copied the preview's capture in (of 50; 0 on a page without the preview)
};

/** The frame's draw time: the best of five batches of ten draws (ms a draw), wall and the thread's own CPU, first at one instant
 *  (the stream preview re-captures every draw), then warm (the clock stepping 1/30 s a draw: the preview's client is captured
 *  15 times a second at a table, 10 on the lobby, and copied in between). Each list is sized from the last, as the game sizes
 *  the laptop's. The clock stays where the warm draws leave it (it only moves forward). */
KBench BenchMs(KDesk& D, const std::string& Name)
{
	KBench Out;
	Out.Name = Name;
	Out.Wall = Out.Cpu = Out.Capture = Out.Max = 1.0e9;
	const DrawList First = D.Frame();
	Out.Vertices = First.Vertices.size();
	size_t HintIndices = First.Indices.size();
	size_t HintCmds = First.Cmds.size();
	long long Cap0 = 0, Copy0 = 0;
	for (int Pass = 0; Pass < 2; ++Pass)
	{
		if (Pass == 1)
		{
			kw::KastCaptureCounts(&D.RL, Cap0, Copy0);
		}
		for (int B = 0; B < 5; ++B)
		{
			const auto T0 = std::chrono::steady_clock::now();
			const double C0 = KastCpuMs();
			double Worst = 0.0;
			for (int K = 0; K < 10; ++K)
			{
				const double D0 = KastCpuMs();
				D.Now += Pass == 1 ? 1.0 / 30.0 : 0.0;
				// Sized from the last frame, as the game's FDrawListHint sizes the laptop's list (SlateDrawList.h).
				DrawList L;
				L.Vertices.reserve(Out.Vertices + Out.Vertices / 8);
				L.Indices.reserve(HintIndices + HintIndices / 8);
				L.Cmds.reserve(HintCmds + HintCmds / 8);
				ss::ui::Canvas C(L, D.M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
				D.RL.Draw(C, D.Now);
				Out.Vertices = L.Vertices.size();
				HintIndices = L.Indices.size();
				HintCmds = L.Cmds.size();
				Worst = std::max(Worst, KastCpuMs() - D0);
			}
			const double Cpu = (KastCpuMs() - C0) / 10.0;
			if (Pass == 1)
			{
				Out.Max = std::min(Out.Max, Worst);
				Out.MaxAll = std::max(Out.MaxAll, Worst);
			}
			(Pass == 1 ? Out.Cpu : Out.Capture) = std::min(Pass == 1 ? Out.Cpu : Out.Capture, Cpu);
			if (Pass == 1)
			{
				Out.Wall = std::min(Out.Wall, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 10.0);
			}
		}
	}
	long long Cap1 = 0, Copy1 = 0;
	kw::KastCaptureCounts(&D.RL, Cap1, Copy1);
	Out.Copies = Copy1 - Copy0;
	return Out;
}

std::vector<KBench> Bench;

void Time(KDesk& D, const std::string& Name)
{
	Bench.push_back(BenchMs(D, Name));
}

/** A few chat lines from people in the room (ids past the stream's own). */
void Chat(KDesk& D, const std::vector<std::pair<std::string, std::string>>& Lines)
{
	int Id = 900000 + static_cast<int>(D.S.Stream.Chat.size());
	for (const auto& Ln : Lines)
	{
		kast::ChatMsg M;
		M.Id = ++Id;
		M.Who = Ln.first;
		M.Text = Ln.second;
		M.Kind = Ln.first.empty() ? kast::LineKind::System : kast::LineKind::Chat;
		M.NameColor = 0x51cf66;
		M.At = D.Now - 2.0;
		D.S.Stream.Chat.push_back(M);
	}
}

/** The night of the mockups' first screen: 12 watching, 5 chatting, three hours into a long MTT, Running hot. */
kast::View MidMtt()
{
	kast::View V;
	V.Count = 12;
	V.ShownCount = 12;
	V.Chatting = 5;
	V.Sources[static_cast<size_t>(kast::View::Source::Members)] = 5;
	V.Sources[static_cast<size_t>(kast::View::Source::Directory)] = 4;
	V.Sources[static_cast<size_t>(kast::View::Source::Raid)] = 2;
	V.Sources[static_cast<size_t>(kast::View::Source::Video)] = 1;
	V.Heat = 38.0;
	V.GhostGain = 3.0;
	V.Tier = 2;
	V.Why = {copy::WhyShowingUp(4), copy::WhyOverForecast(38)};
	V.Rank = 14;
	V.RankOf = 27;
	V.Fc = {8.0, 11.0, 14.0};
	V.NodsLeft = 2;
	return V;
}

/** The final table of the mockups' second screen: 118 watching, the Broadcast in, Hold or Crack open, the rest waiting. */
kast::View FinalTable()
{
	kast::View V;
	V.Count = 118;
	V.ShownCount = 118;
	V.Chatting = 23;
	V.Sources[static_cast<size_t>(kast::View::Source::Members)] = 48;
	V.Sources[static_cast<size_t>(kast::View::Source::Broadcast)] = 33;
	V.Sources[static_cast<size_t>(kast::View::Source::Directory)] = 21;
	V.Sources[static_cast<size_t>(kast::View::Source::Rail)] = 9;
	V.Sources[static_cast<size_t>(kast::View::Source::Lurkers)] = 7;
	V.Heat = 36.0;
	V.GhostGain = 29.0;
	V.Tier = 2;
	V.Why = {copy::WhyRun(ss::kast::growth::RunStage::FinalTable, 980, 0, "Tuesday"), copy::WhyShowingUp(4)};
	V.EventMode = true;
	V.DeepRemaining = 7;
	V.DeepEntrants = 980;
	V.Prestige = 1.40;
	V.DeepEvent = "$11 Big Stack";
	V.Action = kast::View::Slot::HoldCrack;
	V.Waiting = (1 << static_cast<int>(kast::View::Slot::WelcomeRaid)) | (1 << static_cast<int>(kast::View::Slot::RailCall));
	V.RunAlertSent = 256;
	V.RunAlertBack = 12;
	V.HoC.Open = true;
	V.HoC.Holds = 13;
	V.HoC.Cracks = 4;
	V.HoC.Equity = 0.702;
	V.Rank = 6;
	V.RankOf = 31;
	V.Fc = {41.0, 52.0, 63.0};
	V.NodsLeft = 2;
	V.Lit = {"nitbox"};
	return V;
}

void Graph(KDesk& D, const std::vector<float>& Vals)
{
	D.S.Stream.Graph = Vals;
}

const Rect StudioTile0{20.0f, 772.0f, 160.0f, 174.0f};
const Rect StudioChat{1044.0f, 74.0f, 536.0f, 872.0f};
const Rect StudioPrev{20.0f, 74.0f, 1008.0f, 567.0f};

// ------------------------------------------------------------------ K1: one count, every surface

void OneCount()
{
	KDesk D("kast-ui-count");
	for (const char* Mod : {"nitbox", "kat_v", "riverrat_jo"})
	{
		kast::Moderator Md;
		Md.Name = Mod;
		Md.Online = 1.0;
		D.S.Channel.Mods.push_back(Md);
	}
	Check(D.Live(), "count: live");
	D.Studio();
	kast::View V = MidMtt();
	D.S.Stream.DebugView(V);
	D.S.Stream.StartWorld = D.S.WorldMinutes() - 180.0; // three hours in (the mockup's night): the ghost has its number
	Graph(D, {9, 11, 10, 12, 13, 11, 12, 10, 11, 12, 12});
	Chat(D, {{"nitbox", "level up"}, {"mei", "how many left?"}, {"felt_and_found", "late reg still open?"}, {"kat_v", "nice pot"}, {"dee", "the laundromat says hi"},
				{"riverrat_jo", "what would you do with AQ under the gun?"}});
	D.Now += 1.0;
	DrawList L = D.Shot("studio_live");
	Keep(L);
	Time(D, "studio_live");
	Check(RunIn(L, "12", D.M, StudioTile0).Found && RunIn(L, "watching", D.M, StudioTile0).Found, "count: the VIEWERS tile draws the View's 12 watching");
	Check(RunIn(L, copy::SourceCount(kast::View::Source::Members, 5), D.M, StudioTile0, true).Found && RunIn(L, copy::SourceCount(kast::View::Source::Video, 1), D.M, StudioTile0, true).Found,
		"count: where they came from, the top four sources");
	Check(RunIn(L, copy::Chatting(5), D.M, StudioTile0).Found, "count: 5 chatting on the tile");
	Check(RunIn(L, copy::ForecastShort(8.0, 14.0), D.M, StudioTile0).Found, "count: the forecast band on the night's graph");
	Check(RunIn(L, copy::Watching(12), D.M, StudioChat).Found && RunIn(L, copy::Chatting(5), D.M, StudioChat, true).Found, "count: Studio's chat header reads 12 watching · 5 chatting");
	// The Chat tab carries no number (C12): only its word in the tab row's left run.
	bool TabNumber = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const ss::ui::TextItem& T = Cmd.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && T.BaselineY > 100.0f && T.BaselineY < 110.0f && T.X > 1050.0f && T.X < 1114.0f && T.Text != "Chat")
		{
			TabNumber = true;
		}
	}
	Check(!TabNumber, "count: the Chat tab carries no number");
	if (TabNumber)
	{
		DumpIn(L, {1044.0f, 84.0f, 536.0f, 30.0f}, "the chat tabs");
	}
	Check(RunIn(L, copy::RankHeader(14, 27), D.M).Found, "count: the header's Poker rank");
	Check(RunIn(L, copy::ModsHere(D.S.Stream.ModsOnline(), 3), D.M, StudioChat).Found, "count: the Mods tab says who's here (N of 3 here)");
	// The pill in RiverLine's top bar and the directory card draw the same integer.
	D.RL.OpenApp(ss::ui::RiverLine::App::RiverLine, D.Now);
	L = D.Frame();
	Check(RunIn(L, "12", D.M, {1100.0f, 0.0f, 500.0f, 60.0f}).Found && RunIn(L, "KAST", D.M, {1100.0f, 0.0f, 500.0f, 60.0f}).Found, "count: the KAST pill draws 12");
	D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
	D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
	L = D.Frame();
	Check(RunIn(L, "12 viewers", D.M).Found, "count: the directory card draws 12");
	Check(RunIn(L, copy::RankNow(14), D.M).Found, "count: Browse's rank is the Room's");
	// The 24-inch Studio.
	{
		DrawList Ls;
		ss::ui::Canvas Cs(Ls, D.M, ss::ui::secondscreen::Width, ss::ui::secondscreen::Height, 1.0f);
		ss::ui::secondscreen::Draw(Cs, D.S, 0, D.Now);
		WriteList("kast_ui_second_screen", Ls);
		Keep(Ls);
		const std::string Line = copy::WatchingTier(12, 2);
		const size_t Dot = Line.find(" \xC2\xB7 ");
		Check(Dot != std::string::npos && RunIn(Ls, Line.substr(0, Dot), D.M).Found && RunIn(Ls, Line.substr(Dot), D.M).Found, "count: the 24-inch Studio reads 12 watching · Running hot");
	}
	// The tile strip reserves the pill's own measure: KastCount of the target (the 1,000+ mismatch).
	V.Count = 12400;
	D.S.Stream.DebugView(V);
	{
		DrawList Lw;
		ss::ui::Canvas Cw(Lw, D.M, 1600.0f, 1000.0f, 1.0f);
		const float W = kw::KastPillWidth(Cw, D.S);
		Check(std::fabs(W - (Cw.Measure(kw::KastCount(12400), 13.0f, 900, true) + 80.0f)) < 0.01f && kw::KastCount(12400) == "12.4K", "count: the pill's width is KastCount's (12.4K)");
	}
	// Chatting is never above watching, even from a view that says so.
	V = MidMtt();
	V.Count = 3;
	V.ShownCount = 3;
	V.Chatting = 9;
	D.S.Stream.DebugView(V);
	D.Studio();
	D.Now += 1.0;
	L = D.Frame();
	Check(!RunIn(L, copy::Chatting(9), D.M, StudioChat, true).Found, "count: chatting never reads above watching (as the count steps down)");
	D.Now += 0.5;
	L = D.Frame();
	Check(RunIn(L, copy::Chatting(3), D.M, StudioChat, true).Found && !RunIn(L, copy::Chatting(9), D.M, StudioChat, true).Found, "count: chatting never reads above watching");
}

// ------------------------------------------------------------------ the count-up, and the preview's stills

void CountUp()
{
	KDesk D("kast-ui-countup");
	Check(D.Live(), "count-up: live");
	D.Studio();
	kast::View V = MidMtt();
	D.S.Stream.DebugView(V);
	D.Now += 1.0;
	D.Frame();
	D.Now += 1.0;
	D.Frame();
	V.Count = 20;
	V.ShownCount = 12;
	D.S.Stream.DebugView(V);
	DrawList L = D.Frame();
	// The tile counts up from 12 over 0.3 real s; the stream preview (the client's own pill inside it) is a still at 20.
	Check(RunIn(L, "12", D.M, StudioTile0).Found, "count-up: the tile starts from the last shown");
	Check(kw::KastShown(D.S, D.Now, true) == 20, "count-up: a still is the target");
	D.Now += 0.12;
	L = D.Frame();
	const KRun Mid = RunIn(L, "watching", D.M, StudioTile0);
	int Shown = -1;
	for (int N = 12; N <= 20; ++N)
	{
		Shown = RunIn(L, std::to_string(N), D.M, StudioTile0).Found ? N : Shown;
	}
	Check(Mid.Found && Shown > 12 && Shown < 20, "count-up: between the two a moment later");
	D.Now += 0.3;
	L = D.Frame();
	Check(RunIn(L, "20", D.M, StudioTile0).Found, "count-up: the target after 0.3 s");
	// The pill and the second screen share the clock: both read the same integer as the tile.
	V.Count = 7;
	D.S.Stream.DebugView(V);
	D.Frame();
	D.Now += 0.5;
	D.RL.OpenApp(ss::ui::RiverLine::App::RiverLine, D.Now);
	L = D.Frame();
	Check(RunIn(L, "7", D.M, {1100.0f, 0.0f, 500.0f, 60.0f}).Found, "count-up: the pill lands on the same 7");
}

// ------------------------------------------------------------------ K2 R6: the honest zero, Set the scene; K9 the forecast

void HonestZero()
{
	KDesk D("kast-ui-zero");
	Check(D.Live(), "zero: live");
	D.S.Channel.Streams = 14;
	D.S.Channel.Followers = 23;
	D.Studio();
	kast::View V;
	V.Count = 0;
	V.Heat = 27.0;
	V.Tier = 1;
	V.GhostGain = 0.0;
	V.SceneOpen = true;
	V.Rank = 27;
	V.RankOf = 27;
	V.Fc = {1.0, 2.0, 4.0};
	D.S.Stream.DebugView(V);
	Chat(D, {{"", copy::EmptyRising()}, {"", copy::EmptyRank(27, 27)}, {"", copy::EmptySeen(41, 2)}});
	D.Now += 1.0;
	DrawList L = D.Shot("honest_zero");
	Keep(L);
	Check(RunIn(L, "0", D.M, StudioTile0).Found && RunIn(L, copy::NobodyHere(), D.M, StudioTile0).Found && RunIn(L, copy::Chatting(0), D.M, StudioTile0).Found,
		"zero: 0 watching, nobody here right now, 0 chatting");
	const KRun Scene = RunIn(L, copy::SetTheSceneButton(), D.M, {20.0f, 730.0f, 220.0f, 36.0f});
	Check(Scene.Found, "zero: Set the scene in the slot under the Go block");
	ClickRun(D, Scene); // the verb is the Session's (StreamSetScene); the press must not throw anything up
	int Goal = 0;
	const std::string Foot = kw::KastFollowerFoot(D.S.Channel, Goal);
	Check(Goal == 25 && RunIn(L, Foot, D.M, {190.0f, 772.0f, 160.0f, 174.0f}).Found, "zero: the FOLLOWERS tile counts to the real Affiliate bar (2 to go: Affiliate at 25)");
	// Offline: Go live carries the forecast (the Room's band; until it lands, today's estimate).
	D.S.EndStream();
	D.S.StreamCard = false;
	D.Now += 1.0;
	L = D.Shot("offline_forecast");
	Keep(L);
	const kast::growth::Band Fc = D.S.KastForecast();
	Check(Fc.Hi > 0.0 ? RunIn(L, copy::ForecastBand(Fc.Lo, Fc.Hi), D.M).Found : RunIn(L, "viewers to start", D.M, {20.0f, 654.0f, 220.0f, 64.0f}, true).Found,
		"forecast: under Go live (Tonight: likely ..., or today's estimate before the Room's band)");
	Check(RunIn(L, "Go live", D.M, {20.0f, 654.0f, 220.0f, 64.0f}).Found, "forecast: Go live stays where riverline_ui presses it");
	Time(D, "studio_offline");
}

// ------------------------------------------------------------------ K2 First Faces, First Follower (the first stream)

void FirstStream()
{
	KDesk D("kast-ui-first");
	Check(D.Live(), "first: live");
	D.Studio();
	kast::View V;
	V.Count = 4;
	V.ShownCount = 4;
	V.Chatting = 3;
	V.Sources[static_cast<size_t>(kast::View::Source::Members)] = 2;
	V.Sources[static_cast<size_t>(kast::View::Source::Fresh)] = 2;
	V.Heat = 26.0;
	V.GhostGain = 2.0;
	V.Tier = 1;
	V.Why = {copy::WhyNew(), std::string()};
	V.Rank = 22;
	V.RankOf = 26;
	V.Fc = {2.0, 3.0, 5.0};
	V.PingReached = 2;
	V.PingOf = 2;
	D.S.Stream.DebugView(V);
	Graph(D, {0, 1, 2, 2, 3, 3, 4, 4, 3, 4, 4});
	Chat(D, {{"", copy::GoLive(2, 2)}, {"", copy::EmptyFresh()}, {"dee", copy::DeeHi()}, {"mei", copy::MeiHi()}, {"nightrail_88", "first time here"}, {"foldcity_ann", "hello from Ohio"},
				{"", copy::FirstFollower("nightrail_88")}});
	kast::Alert A;
	A.Kind = kast::AlertKind::Follow;
	A.Who = "nightrail_88";
	A.Text = copy::FirstFollowerSub();
	A.At = D.Now - 0.2;
	D.S.Stream.Alerts.insert(D.S.Stream.Alerts.begin(), A);
	D.Frame(); // the toast goes up
	D.Now += 1.0;
	DrawList L = D.Shot("first_stream");
	Keep(L);
	Check(RunIn(L, "nightrail_88", D.M, StudioPrev, true).Found && RunIn(L, "isn't", D.M, StudioPrev, true).Found, "first: the first follower's alert says who and why");
	Check(RunIn(L, copy::SourceCount(kast::View::Source::Fresh, 2), D.M, StudioTile0, true).Found, "first: 2 from Fresh on Kast on the tile");
	Check(RunIn(L, copy::PingToast(2, 2), D.M, StudioChat).Found, "first: the Studio toast reports the ping's reach");
	// The night hasn't settled a world minute in (C-K's M4): the ghost has no number yet.
	Check(RunIn(L, copy::TierName(1), D.M, {660.0f, 772.0f, 170.0f, 174.0f}).Found && RunIn(L, copy::GhostSettling(), D.M, {660.0f, 772.0f, 170.0f, 174.0f}).Found &&
			  !RunIn(L, copy::GhostTonight(2.0), D.M, {660.0f, 772.0f, 170.0f, 174.0f}).Found,
		"first: Warming up, tonight: settling in (no number in the night's first minutes)");
	int Goal = 0;
	Check(RunIn(L, kw::KastFollowerFoot(D.S.Channel, Goal), D.M, {190.0f, 772.0f, 160.0f, 174.0f}).Found, "first: the FOLLOWERS tile counts to Affiliate's 25");
}

// ------------------------------------------------------------------ K4 Heat

void Heat()
{
	KDesk D("kast-ui-heat");
	D.S.WorldSkip(12); // a world far enough in for nine dark days
	ss::kast::Channel& Ch = D.S.Channel;
	const double World = D.S.WorldMinutes();
	const int Today = ss::net::DayOf(World);
	Ch.Streams = 40;
	Ch.Heat = 41.0;
	Ch.HeatAt = World;
	Ch.LastStreamDay = Today - 9;
	D.Studio();
	DrawList L = D.Shot("heat_offline");
	Keep(L);
	const double Shown = ss::kast::growth::HeatShown(Ch.Heat, Ch.HeatAt, World, Ch.LastStreamDay);
	const int Tier = ss::kast::growth::Tier(Shown);
	Check(RunIn(L, copy::TierName(Tier), D.M, {772.0f, 0.0f, 828.0f, 60.0f}).Found, "heat: the header chip's tier word (offline, the channel's)");
	Check(RunIn(L, copy::TierName(Tier), D.M, {660.0f, 772.0f, 160.0f, 174.0f}).Found, "heat: tile 4 is HEAT");
	const bool Cooling = RunIn(L, copy::WhyCooling(9), D.M, {660.0f, 772.0f, 170.0f, 174.0f}, true).Found || RunIn(L, "Cooling", D.M, {660.0f, 772.0f, 170.0f, 174.0f}, true).Found;
	Check(Cooling, "heat: the top why line offline (Cooling: 9 days dark)");
	if (!Cooling)
	{
		DumpIn(L, {660.0f, 772.0f, 170.0f, 174.0f}, "the HEAT tile");
	}
	// L8: the same channel as an old save before its first stream here (no U3Kast, no Heat line of its own, its days live kept,
	// the last nine world days back): the chip and the tile read the Heat its go-live will seed (15, Cold) and the why line that
	// night starts from, not the start an unmigrated channel would read (26, Warming up).
	{
		const std::vector<int> DaysWas = Ch.DaysLive;
		const int LastWas = Ch.LastStreamDay;
		D.S.Life.Update3LinesRead &= ~static_cast<uint32_t>(ss::life::U3Kast);
		Ch.DaysLive = {Today - 12, Today - 9};
		Ch.LastStreamDay = -1;
		D.Now += 1.0;
		const DrawList OL = D.Frame();
		Check(RunIn(OL, copy::TierName(0), D.M, {772.0f, 0.0f, 828.0f, 60.0f}).Found && RunIn(OL, copy::TierName(0), D.M, {660.0f, 772.0f, 160.0f, 174.0f}).Found &&
				  RunIn(OL, copy::WhyCooling(9), D.M, {660.0f, 772.0f, 170.0f, 174.0f}, true).Found,
			"heat: an old save before its first stream reads the Heat its go-live seeds: Cold, Cooling: 9 days dark (L8)");
		Ch.DaysLive = DaysWas;
		Ch.LastStreamDay = LastWas;
		D.S.Life.Update3LinesRead |= ss::life::U3Kast;
	}
	// Live: the ghost fill's line and the tier from the View.
	Check(D.Live(), "heat: live");
	D.Studio();
	kast::View V = FinalTable();
	V.EventMode = false;
	V.HoC.Open = false;
	V.Action = kast::View::Slot::None;
	V.Tier = 3;
	V.Heat = 58.0;
	D.S.Stream.DebugView(V);
	D.S.Stream.StartWorld = D.S.WorldMinutes() - 90.0; // an hour and a half in: the night has settled (M4)
	D.Now += 1.0;
	L = D.Frame();
	Check(RunIn(L, copy::TierName(3), D.M, {660.0f, 772.0f, 160.0f, 174.0f}).Found && RunIn(L, copy::GhostTonight(29.0), D.M, {660.0f, 772.0f, 160.0f, 174.0f}).Found,
		"heat: live, the tier and tonight +29 so far");
	// Never a flame: no fire word on any Kast page (screened at the end over every page).
}

// ------------------------------------------------------------------ K5 K12 K6 K3: Event Mode, Hold or Crack, the alert, the slot

void EventMode()
{
	KDesk D("kast-ui-event");
	Check(D.Live(), "event: live");
	D.Studio();
	kast::View V = FinalTable();
	D.S.Stream.DebugView(V);
	D.S.Stream.StartWorld = D.S.WorldMinutes() - 401.1; // the mockup's 6:41:07 live: the ghost reads +29
	Graph(D, {6, 22, 38, 44, 47, 50, 49, 52, 55, 51, 54, 58, 61, 60, 64, 66, 71, 79, 84, 88, 97, 104, 112, 118});
	kast::Alert A;
	A.Kind = kast::AlertKind::Raid;
	A.Who = "RiverLine Live";
	A.Count = 33;
	A.Text = copy::Broadcast(0, 33); // the Broadcast in its site's voice (patches/ui-kast_to_core-kastroom_1.diff puts it there)
	A.At = D.Now - 1.0;
	D.S.Stream.Alerts.insert(D.S.Stream.Alerts.begin(), A);
	Chat(D, {{"", copy::RunAlertSent(256)}, {"kat_v", "you're at the final table??"}, {"lowball_lou", "back for the FT!"}, {"", copy::Broadcast(0, 33)}, {"nitbox", "he noticed me kastLove"},
				{"mei", "ICM spot here"}, {"ajax_folds", "THIS IS IT"}});
	D.Now += 0.1;
	DrawList L = D.Shot("studio_event");
	Keep(L);
	Time(D, "studio_event");
	const bool Final = RunIn(L, copy::EventBar(true, "$11 Big Stack", 7, 980), D.M, StudioPrev).Found;
	Check(Final || RunIn(L, copy::EventBar(false, "$11 Big Stack", 7, 980), D.M, StudioPrev).Found, "event: the EVENT bar across the preview");
	const KRun Live = RunIn(L, "LIVE  ", D.M, {900.0f, 0.0f, 700.0f, 60.0f}, true);
	Check(Live.Found && SameColour(Live.Col, kw::KDeep), "event: the LIVE pill turns gold (its words dark)");
	Check(RunIn(L, copy::HoldOrCrackTitle(), D.M, StudioPrev).Found && RunIn(L, copy::HoldOrCrackShares(13, 4), D.M, StudioPrev).Found,
		"event: the preview's Hold or Crack bar (shares from 10 callers)");
	// The alert sits under the EVENT bar and the Hold or Crack bar (the mockup's collision).
	const KRun Hoc = RunIn(L, copy::HoldOrCrackTitle(), D.M, StudioPrev);
	const KRun Raid = RunIn(L, kw::KastBroadcastHeadline(), D.M, StudioPrev);
	Check(Raid.Found && Hoc.Found && Raid.Base - Raid.Size > StudioPrev.Y + 84.0f, "event: the alert clears the Hold or Crack bar and the EVENT bar");
	Check(std::string(kw::KastBroadcastHeadline()) == "THE BROADCAST" && !RunIn(L, "RAID!", D.M, StudioPrev).Found, "event: the Broadcast's headline reads THE BROADCAST, not RAID! (C-K's L1)");
	Check(RunIn(L, "tablists", D.M, StudioPrev, true).Found && RunIn(L, "sent", D.M, StudioPrev, true).Found, "event: the Broadcast's alert speaks in its site's voice (word by word, as alerts draw)");
	Check(RunIn(L, "nitbox", D.M, StudioPrev).Found, "event: a Nod's lit name on the overlay");
	Check(RunIn(L, kw::KastCount(118), D.M, StudioTile0).Found, "event: 118 watching on the tile");
	// The Studio's slot holds Hold or Crack's calls.
	Check(RunIn(L, "HOLDS", D.M, {20.0f, 730.0f, 220.0f, 36.0f}).Found && RunIn(L, "CRACKS", D.M, {20.0f, 730.0f, 220.0f, 36.0f}).Found, "event: Studio's slot calls Hold or Crack");
	// The directory card's FINAL TABLE chip and the Sweating now shelf.
	D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
	L = D.Shot("browse_event");
	Keep(L);
	Check(RunIn(L, copy::DirectoryEvent("$11 Big Stack", 7), D.M).Found, "event: the directory card's FINAL TABLE chip");
	Check(RunIn(L, copy::SweatingNowShelf(), D.M).Found && RunIn(L, copy::BroadcastTease(0), D.M).Found, "event: Sweating now with the Broadcast's tease");
	Time(D, "browse");
}

/** StreamSide at a real table: the strip, Hold or Crack, the slot, the chips, the Run Alert's counter, heads-down, the toast. */
void TableSide()
{
	KDesk D("kast-ui-table");
	Check(D.Live(), "side: live");
	D.Now = Step(D.S, D.Now, 10.0);
	const ss::net::Network& Net = ss::net::Shared();
	const double World = D.S.WorldMinutes();
	std::vector<ss::LobbyEvent> Picks;
	for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 60.0))
	{
		const ss::LobbyEvent Lb = Net.Listing(E, nullptr, D.S.Unlocks());
		if (Lb.Joinable && Lb.BuyInCents <= 1100 && Lb.Spec.Entrants <= 3000 && Picks.empty())
		{
			Picks.push_back(Lb);
		}
	}
	Check(!Picks.empty(), "side: an event to stream");
	if (Picks.empty())
	{
		return;
	}
	D.S.RegisterEvent(Picks[0]);
	for (int K = 0; K < 30 && D.S.CurrentScreen != ss::Screen::Table; ++K)
	{
		D.Now = Step(D.S, D.Now, 20.0);
	}
	Check(D.S.CurrentScreen == ss::Screen::Table, "side: seated at a table");
	D.RL.OpenApp(ss::ui::RiverLine::App::RiverLine, D.Now);
	D.S.Tab = ss::RightTab::Stream;
	kast::View V = FinalTable();
	D.S.Stream.DebugView(V);
	DrawList L = D.Shot("table_event");
	Keep(L);
	Time(D, "table_event");
	const Rect Side{1180.0f, 60.0f, 420.0f, 900.0f};
	Check(RunIn(L, kw::KastCount(118), D.M, Side).Found && RunIn(L, copy::Chatting(23), D.M, Side, true).Found, "side: 118 watching · 23 chatting in the strip");
	Check(RunIn(L, copy::RunAlertStrip(12), D.M, Side).Found, "side: the Run Alert's counter");
	Check(RunIn(L, copy::HoldOrCrackTitle(), D.M, Side).Found && RunIn(L, copy::YourEquity(0.702), D.M, Side).Found, "side: Hold or Crack with the equity marker");
	Check(RunIn(L, copy::CallIt(true), D.M, Side).Found && RunIn(L, copy::CallIt(false), D.M, Side).Found, "side: Call it: HOLDS and Call it: CRACKS in the slot");
	Check(RunIn(L, "waiting:", D.M, Side).Found && RunIn(L, copy::WelcomeTheRaidButton(), D.M, Side).Found && RunIn(L, copy::RailCallButton(), D.M, Side).Found,
		"side: Welcome the raid and Rail Call wait as chips");
	Check(RunIn(L, copy::TierName(2), D.M, Side).Found, "side: the tier word beside the count");
	const KRun Pill = RunIn(L, "KAST", D.M, {1100.0f, 0.0f, 500.0f, 60.0f});
	Check(Pill.Found && SameColour(Pill.Col, kw::KDeep), "side: the KAST pill is gold in Event Mode");
	// Counts under 10 callers.
	V.HoC.Holds = 2;
	V.HoC.Cracks = 1;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::HoldOrCrackCounts(2, 1), D.M, Side).Found, "side: counts under 10 callers (2 HOLDS · 1 CRACKS)");
	// Nobody has called yet: the counts over an empty track, never a 50/50 split chat didn't make. (The bar sits 5-14 units
	// under the strip's title; its lime and red are the split.)
	{
		auto SplitVerts = [&](const DrawList& Ld) {
			const KRun T = RunIn(Ld, copy::HoldOrCrackTitle(), D.M, Side);
			int N = 0;
			for (const ss::ui::Vertex& Vx : Ld.Vertices)
			{
				const bool InBar = Vx.X >= Side.X && Vx.X <= Side.X + Side.W && Vx.Y >= T.Base + 4.0f && Vx.Y <= T.Base + 15.0f;
				N += InBar && (SameColour(Vx.Col, kw::KLime) || SameColour(Vx.Col, kw::KRed)) ? 1 : 0;
			}
			return T.Found ? N : -1;
		};
		const int SplitAt21 = SplitVerts(L);
		V.HoC.Holds = 0;
		V.HoC.Cracks = 0;
		D.S.Stream.DebugView(V);
		L = D.Shot("table_hoc_nocalls");
		const int SplitAt0 = SplitVerts(L);
		Check(SplitAt21 > 0 && SplitAt0 == 0 && RunIn(L, copy::HoldOrCrackCounts(0, 0), D.M, Side).Found, "side: no calls yet: the counts over an empty track, no 50/50 split");
	}
	// The edge (§5.3 K12): 9 callers still read as counts, 10 as shares.
	V.HoC.Holds = 6;
	V.HoC.Cracks = 3;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::HoldOrCrackCounts(6, 3), D.M, Side).Found, "side: 9 callers read as counts");
	V.HoC.Holds = 7;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::HoldOrCrackShares(7, 3), D.M, Side).Found && !RunIn(L, copy::HoldOrCrackCounts(7, 3), D.M, Side).Found, "side: 10 callers read as shares");
	// The streamer's call: the press goes to the Session's verb (StreamCallIt), the other button doesn't double it.
	ClickRun(D, RunIn(L, copy::CallIt(true), D.M, Side));
	// Rail Call alone in the slot, then its reach, then the whole rail.
	V = FinalTable();
	V.HoC.Open = false;
	V.Action = kast::View::Slot::RailCall;
	V.Waiting = 0;
	D.S.Stream.DebugView(V);
	L = D.Shot("table_railcall");
	Check(RunIn(L, copy::RailCallButton(), D.M, Side).Found && !RunIn(L, "waiting:", D.M, Side).Found, "side: RAIL CALL in the slot, nothing waiting");
	// The slot's own action never waits as a chip too, even when the Room's Waiting holds its bit.
	V.Waiting = (1 << static_cast<int>(kast::View::Slot::RailCall)) | (1 << static_cast<int>(kast::View::Slot::WelcomeRaid));
	D.S.Stream.DebugView(V);
	L = D.Frame();
	{
		int RailRuns = 0;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			RailRuns += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == copy::RailCallButton() && Cmd.Text.X >= Side.X ? 1 : 0;
		}
		Check(RailRuns == 1 && RunIn(L, "waiting:", D.M, Side).Found && RunIn(L, copy::WelcomeTheRaidButton(), D.M, Side).Found, "side: the slot's action isn't a waiting chip as well");
	}
	V.Waiting = 0;
	V.Action = kast::View::Slot::None;
	V.RailReach = 37;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::RailCallReached(37), D.M, Side).Found, "side: Your rail call reached 37 followers");
	V.RailReach = 0;
	V.WholeRail = true;
	V.HeadsDown = true;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::WholeRailHere(), D.M, Side).Found, "side: Your whole rail is here");
	Check(RunIn(L, copy::HeadsDown(), D.M, Side).Found, "side: heads-down tonight under the count");
	// Welcome the raid in the slot.
	V.WholeRail = false;
	V.HeadsDown = false;
	V.Action = kast::View::Slot::WelcomeRaid;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Keep(L);
	Check(RunIn(L, copy::WelcomeTheRaidButton(), D.M, Side).Found, "side: WELCOME THE RAID in the slot");
	// Its countdown (C-K's L11: View::WelcomeRaidSecs), in the slot and on the waiting chip (the mockup's WELCOME THE RAID 1:52).
	V.WelcomeRaidSecs = 112;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, copy::WelcomeTheRaidButton() + " 1:52", D.M, Side).Found, "side: WELCOME THE RAID 1:52 in the slot");
	V.Action = kast::View::Slot::RailCall;
	V.Waiting = 1 << static_cast<int>(kast::View::Slot::WelcomeRaid);
	D.S.Stream.DebugView(V);
	L = D.Shot("table_welcome_chip");
	Keep(L);
	Check(RunIn(L, copy::WelcomeTheRaidButton() + " 1:52", D.M, Side).Found && RunIn(L, "waiting:", D.M, Side).Found, "side: the waiting chip counts down too");
	V.Action = kast::View::Slot::WelcomeRaid;
	V.Waiting = 0;
	V.WelcomeRaidSecs = 0;
	D.S.Stream.DebugView(V);
	// The toast line (11.5-unit type under the slot; chat's own system lines are bigger): Front Row, and never two inside a real
	// minute.
	auto Toast = [&](const DrawList& Ld, const std::string& Text) {
		for (const ss::ui::DrawCmd& Cmd : Ld.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && std::fabs(Cmd.Text.SizePx - 11.5f) < 0.05f && Cmd.Text.X > Side.X)
			{
				return true;
			}
		}
		return false;
	};
	Chat(D, {{"", copy::FrontRow()}, {"", copy::WelcomeBack(14)}});
	V.Action = kast::View::Slot::None;
	D.S.Stream.DebugView(V);
	L = D.Shot("table_toast");
	Check(Toast(L, copy::FrontRow()), "side: Front Row on the toast line");
	if (!Toast(L, copy::FrontRow()))
	{
		DumpIn(L, Side, "StreamSide");
	}
	D.Now += 10.0;
	L = D.Frame();
	Check(!Toast(L, copy::WelcomeBack(14)) && !Toast(L, copy::FrontRow()), "side: one toast a real minute (Welcome back waits)");
	D.Now += 55.0;
	L = D.Frame();
	Check(Toast(L, copy::WelcomeBack(14)), "side: the next toast a minute later");
	// A toast already up steps down the moment Hold or Crack opens (none while it is, §5.1), and a game's own toast (a milestone)
	// takes the line's: the two never stack.
	{
		D.S.Stream.DebugView(FinalTable());
		D.Now += 1.0;
		L = D.Frame();
		Check(!Toast(L, copy::WelcomeBack(14)), "side: Hold or Crack takes an up toast down");
		D.S.Stream.DebugView(V);
		D.Now += 0.5;
		L = D.Frame();
		kast::Alert G;
		G.Kind = kast::AlertKind::Milestone;
		G.Count = 25;
		G.At = 1.0e6;
		D.S.Stream.Feed.insert(D.S.Stream.Feed.begin(), G);
		L = D.Frame();
		Check(!Toast(L, copy::WelcomeBack(14)), "side: a game's own toast takes the line's (never two at once)");
	}
	// Hold or Crack open: no toast goes up while it is.
	Chat(D, {{"", copy::FrontRow()}});
	V = FinalTable();
	D.S.Stream.DebugView(V);
	D.Now += 70.0;
	L = D.Frame();
	Check(!Toast(L, copy::FrontRow()), "side: no toast while Hold or Crack is open");
	// C-K's L13: the long "Prediction started: ..." system line wraps inside StreamSide's chat, and a word too long for a row of its
	// own ends in an ellipsis; no row runs past the panel's inner edge (each system row is drawn 8 in from the chat body's sides).
	{
		const std::vector<kast::ChatMsg> Was = D.S.Stream.Chat;
		D.S.Stream.Chat.clear();
		const std::string Pred = "Prediction started: Will " + D.S.HeroName + " cash in $11 Big Stack Bounty Builder Deepstack Turbo?";
		Chat(D, {{"", Pred}, {"", "Prediction started: Will " + std::string(70, 'w') + "?"}});
		L = D.Shot("table_prediction");
		const KRun Head = RunIn(L, "Prediction started: Will " + D.S.HeroName, D.M, Side, true);
		const KRun Tail = RunIn(L, "Turbo?", D.M, Side, true);
		bool Ellipsis = false;
		float Right = 0.0f;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			const ss::ui::TextItem& T = Cmd.Text;
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Head.Found && T.X > Side.X && T.BaselineY >= Head.Base - 1.0f && T.BaselineY <= Head.Base + 90.0f)
			{
				Ellipsis = Ellipsis || T.Text.find("\xE2\x80\xA6") != std::string::npos;
				Right = std::max(Right, T.X + D.M.Width(T.Text, T.Face, T.SizePx));
			}
		}
		// The chat body is StreamSide's rect less 8 a side (RiverLine::StreamChat); system rows end 8 in from it.
		const float Inner = 1188.0f + 396.0f - 8.0f - 8.0f;
		Check(Head.Found && Tail.Found && Tail.Base > Head.Base && Ellipsis && Right <= Inner + 0.5f, "side: the long Prediction line wraps, an overlong word ends in an ellipsis, every row inside the panel");
		if (!(Right <= Inner + 0.5f))
		{
			std::printf("  side: a system row runs to %.1f past %.1f\n", Right, Inner);
		}
		D.S.Stream.Chat = Was;
	}
	// The stream preview draws stills: Studio's tile counts up from 118 while StreamSide inside the preview is already at 130.
	D.Studio();
	D.Frame();
	V.Count = 130;
	V.ShownCount = 118;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, "118", D.M, StudioTile0).Found && RunIn(L, "130", D.M, StudioPrev).Found, "side: the preview's StreamSide is a still at the target while the tile counts up");
	// At a table the preview's client is re-captured every other draw at the laptop's 30 (15 a second), copied in between.
	{
		long long C0 = 0, P0 = 0, C1 = 0, P1 = 0, C2 = 0, P2 = 0;
		D.Now += 1.0;
		D.Frame(); // captured: a second on
		kw::KastCaptureCounts(&D.RL, C0, P0);
		D.Now += 1.0 / 30.0;
		D.Frame();
		kw::KastCaptureCounts(&D.RL, C1, P1);
		D.Now += 1.0 / 30.0;
		D.Frame();
		kw::KastCaptureCounts(&D.RL, C2, P2);
		Check(D.S.CurrentScreen == ss::Screen::Table && C1 == C0 && P1 == P0 + 1 && C2 == C1 + 1 && P2 == P1, "capture: at a table the Studio's preview re-captures every other draw (15 a second)");
	}
	// The Studio at a table (C-K's M5: the bench the gate lacked), the final table's view with Hold or Crack open.
	D.Shot("studio_table");
	Time(D, "studio_table");
}

// ------------------------------------------------------------------ K11: the name menu, Nod, Thank's taper

void NameMenu()
{
	KDesk D("kast-ui-nod");
	Check(D.Live(), "nod: live");
	D.Studio();
	kast::View V = MidMtt();
	V.Thanks10 = 2;
	D.S.Stream.DebugView(V);
	Chat(D, {{"riverrat_jo", "what would you do with AQ under the gun?"}, {"nitbox", "seat 7 plays every hand"}});
	D.Now += 1.0;
	DrawList L = D.Frame();
	Check(RunIn(L, "Talk to chat " + copy::ThankHeard(), D.M, StudioChat).Found, "nod: Talk to chat (chat heard you) after two thanks in ten world minutes");
	const KRun Name = RunIn(L, "nitbox:", D.M, StudioChat);
	Check(Name.Found, "nod: the chatter's name");
	ClickRun(D, Name);
	L = D.Shot("studio_nod_menu");
	Keep(L);
	Check(RunIn(L, copy::NodButton(), D.M, StudioChat).Found && RunIn(L, copy::NodsLeft(2), D.M, StudioChat).Found && RunIn(L, "Timeout", D.M, StudioChat).Found &&
			  RunIn(L, "Promote to mod", D.M, StudioChat).Found,
		"nod: the menu (Nod, 2 Nods left, Timeout, Promote to mod)");
	D.Click(600.0f, 300.0f);
	L = D.Frame();
	Check(!RunIn(L, copy::NodButton(), D.M, StudioChat).Found, "nod: a press elsewhere closes it");
	// A lit name glows on the overlay and its line.
	V.Lit = {"nitbox"};
	D.S.Stream.DebugView(V);
	L = D.Frame();
	Check(RunIn(L, "nitbox", D.M, StudioPrev).Found, "nod: the lit name on the overlay");
}

// ------------------------------------------------------------------ K17 K20 K15: the end card

kast::Summary BigNight()
{
	kast::Summary L;
	L.Valid = true;
	L.Minutes = 432.67;
	L.Avg = 74;
	L.Peak = 131;
	L.PeakAt = 380.0;
	L.PeakMoment = static_cast<int>(kast::Moment::FinalTable);
	L.Follows = 46;
	L.Subs = 4;
	L.Gifted = 2;
	L.Chatted = 61;
	L.Returning = 52;
	L.NewFaces = 188;
	L.NewRegulars = 3;
	L.OnSchedule = true;
	L.Sources[static_cast<size_t>(kast::View::Source::Members)] = 64;
	L.Sources[static_cast<size_t>(kast::View::Source::Directory)] = 71;
	L.Sources[static_cast<size_t>(kast::View::Source::Broadcast)] = 33;
	L.Sources[static_cast<size_t>(kast::View::Source::Rail)] = 28;
	L.Sources[static_cast<size_t>(kast::View::Source::Video)] = 4;
	L.FollowSources[static_cast<size_t>(kast::View::Source::Directory)] = 21;
	L.FollowSources[static_cast<size_t>(kast::View::Source::Rail)] = 9;
	L.FollowSources[static_cast<size_t>(kast::View::Source::Word)] = 7;
	L.Regulars = {"kat_v", "nitbox", "mei", "riverrat_jo", "dee"};
	L.HeatBefore = 36.0;
	L.GainSoFar = 29.0;
	L.BestCaller = "kat_v";
	L.BestCalls = 4;
	L.BestCallsOf = 5;
	L.RaidsHeld = {{"RiverLine Live", 33, 14, 3, true}, {"pocketqueens_pk", 12, 5, 1, false}};
	L.RankStart = 31;
	L.RankBest = 6;
	L.TipCents = 2400;
	L.BitCents = 1100;
	L.SubCents = 1500;
	L.AdCents = 520;
	L.RenewCents = 3800;
	L.SponsorPaid = {{"tunnelrat", 1, 1000, 0}};
	L.BestClip = "eights hold at the final table";
	L.BestClipLane = true;
	L.Fc = {41.0, 52.0, 63.0};
	return L;
}

void EndCard()
{
	KDesk D("kast-ui-card");
	ss::kast::Channel& Ch = D.S.Channel;
	Ch.Followers = 462;
	Ch.Affiliate = true;
	Ch.Streams = 74;
	D.S.Stream.Last = BigNight();
	D.S.StreamCard = true;
	D.Studio();
	DrawList L = D.Shot("end_card");
	Keep(L);
	Time(D, "end_card");
	const kast::Summary& S = D.S.Stream.Last;
	Check(RunIn(L, "131", D.M).Found && RunIn(L, "at the final table", D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found, "card: the peak first (131 at the final table)");
	const KRun Peak = RunIn(L, "131", D.M);
	const KRun Avg = RunIn(L, "74", D.M);
	Check(Peak.Found && Avg.Found && Peak.Base < Avg.Base, "card: the peak opens the card, above the night's numbers");
	std::string Verdict = copy::ForecastVerdict(74.0, S.Fc);
	for (char& Up : Verdict)
	{
		Up = static_cast<char>(std::toupper(static_cast<unsigned char>(Up)));
	}
	Check(Verdict == "ABOVE YOUR FORECAST" && RunIn(L, Verdict, D.M).Found, "card: above your forecast");
	Check(RunIn(L, copy::RollCall(S.Regulars), D.M).Found, "card: the regulars' roll call");
	Check(RunIn(L, copy::HeatMove(2, ss::kast::growth::Tier(65.0), 29.0), D.M).Found, "card: Heat's move");
	Check(RunIn(L, copy::BestCaller("kat_v", 4, 5), D.M).Found, "card: the night's best caller");
	Check(RunIn(L, copy::BroadcastRetention("RiverLine Live", 33, 14, 3), D.M).Found && !RunIn(L, copy::RaidRetention("RiverLine Live", 33, 14, 3), D.M).Found &&
			  RunIn(L, copy::RaidRetention("pocketqueens_pk", 12, 5, 1), D.M).Found,
		"card: raids with their 15-minute retention (the Broadcast says sent)");
	Check(RunIn(L, copy::RankClimb(31, 6), D.M).Found, "card: the rank climb");
	Check(RunIn(L, copy::TipsTonight(2400, 1100, true), D.M).Found && RunIn(L, copy::RenewalsAway(3800), D.M).Found, "card: tonight's tips apart from renewals");
	const kast::Sponsor* Tr = kast::FindSponsor("tunnelrat");
	Check(Tr && RunIn(L, copy::SponsorReadLabel(Tr->Brand), D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found, "card: the sponsor read on its own line");
	Check(RunIn(L, copy::NextGoal(38, 500), D.M).Found, "card: the next goal, partly filled");
	Check(RunIn(L, copy::ClipOfTheNight(S.BestClip, true), D.M).Found, "card: Clip of the night, in the Kast feed");
	Check(RunIn(L, copy::CameBecauseOf(), D.M).Found && RunIn(L, copy::SourceCause(kast::View::Source::Directory) + " 71", D.M).Found, "card: where they came from");
	Check(RunIn(L, copy::DoneButton(), D.M).Found, "card: Done");
	Check(Overflows(L, {440.0f, 62.0f, 720.0f, 900.0f}, D.M, "end card") == 0, "card: every word inside the card");
	// Clip of the Night -> Cut (K15): with the clip's moment among Rerun's candidates, Make it a Cut opens Studio on it.
	{
		const double At = D.S.WorldMinutes() - 30.0;
		kast::Clip Cl;
		Cl.Title = S.BestClip;
		Cl.At = At;
		D.S.Channel.Clips.insert(D.S.Channel.Clips.begin(), Cl);
		ss::video::Candidate Cand;
		Cand.Id = "kast-ui-clip";
		Cand.At = At;
		Cand.Source = 0;
		Cand.K = ss::video::Kind::BigPot;
		Cand.Expires = D.S.WorldMinutes() + 10000.0;
		D.S.Life.Video.Open.push_back(Cand);
		L = D.Shot("end_card_cut");
		const KRun Cut = RunIn(L, copy::MakeItACutButton(), D.M);
		Check(Cut.Found, "card: Make it a Cut when the clip's moment is a Rerun candidate");
		ClickRun(D, Cut);
		Check(D.RL.CurrentApp() == ss::ui::RiverLine::App::Video && !D.S.StreamCard, "card: Make it a Cut opens Rerun's Studio and the card goes");
	}
	// Before Affiliate and the second rent: a tip by name, pending money, Clip it; the empty blocks collapse.
	KDesk E("kast-ui-card-first");
	E.S.Channel.Followers = 3;
	E.S.Channel.Streams = 1;
	E.S.Channel.PendingCents = 8600;
	kast::Summary F;
	F.Valid = true;
	F.Minutes = 74.0;
	F.Avg = 3;
	F.Peak = 4;
	F.PeakAt = 31.0;
	F.Follows = 1;
	F.TipCents = 500;
	F.Chatted = 4;
	F.ClipItOpen = true;
	E.S.Stream.Last = F;
	kast::Alert Tip;
	Tip.Kind = kast::AlertKind::Tip;
	Tip.Who = "dee";
	Tip.Cents = 500;
	E.S.Stream.Feed.push_back(Tip);
	E.S.StreamCard = true;
	E.Studio();
	L = E.Shot("end_card_first");
	Keep(L);
	Check(RunIn(L, copy::Tipped("dee", 500), E.M).Found, "card: before Affiliate, dee tipped $5");
	Check(RunIn(L, copy::PendingLine(8600), E.M).Found, "card: pending until the second rent");
	Check(RunIn(L, copy::ClipItButton(), E.M).Found, "card: Clip it when no clip was made");
	Check(!RunIn(L, copy::CameBecauseOf(), E.M).Found && !RunIn(L, "sweated", E.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found && !RunIn(L, copy::BestCaller("x", 1, 1).substr(1), E.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found,
		"card: empty blocks collapse (no sources, no roll call, no caller)");
	Check(Overflows(L, {440.0f, 62.0f, 720.0f, 900.0f}, E.M, "end card (first)") == 0, "card: the short card's words fit");
}

// ------------------------------------------------------------------ K7: Browse's chips and shelves

void Browse()
{
	KDesk D("kast-ui-browse");
	D.S.Channel.Streams = 8;
	D.S.Channel.Norm = 6.0;
	Check(D.Live(), "browse: live");
	kast::View V = MidMtt();
	V.Count = 41;
	V.ShownCount = 41;
	V.Rank = 8;
	V.RankOf = 31;
	D.S.Stream.DebugView(V);
	D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
	D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
	D.Now += 1.0;
	DrawList L = D.Shot("browse_rank");
	Keep(L);
	Check(RunIn(L, copy::RankNow(8), D.M).Found, "browse: You're #8 in Poker right now");
	const int Streams = D.S.Channel.Streams;
	Check(RunIn(L, copy::NewOnKastChip(), D.M).Found == (Streams >= 1 && Streams <= 12), "browse: NEW ON KAST on streams 1-12");
	Check(RunIn(L, copy::RisingChip(), D.M).Found == (Streams > 3 && D.S.Channel.Norm < 15.0), "browse: RISING IN POKER while the usual is under 15");
	Check(RunIn(L, copy::FreshShelf(), D.M).Found && RunIn(L, copy::RisingShelf(), D.M).Found && RunIn(L, copy::SweatingNowShelf(), D.M).Found, "browse: the three shelves");
	Check(RunIn(L, copy::EmptyFresh(), D.M).Found, "browse: you're on the Fresh shelf tonight");
}

// ------------------------------------------------------------------ K3 K4 K10 K16 K20 C8: the Community page

void Community()
{
	KDesk D("kast-ui-community");
	D.S.WorldSkip(12); // a world far enough in for a month of streams and a raid three days ago
	ss::kast::Channel& Ch = D.S.Channel;
	const double World = D.S.WorldMinutes();
	const int Today = ss::net::DayOf(World);
	Ch.Followers = 618;
	Ch.Streams = 40;
	Ch.Affiliate = true;
	Ch.Streak = 6;
	Ch.Heat = 41.0;
	Ch.HeatAt = World;
	Ch.LastStreamDay = Today - 1;
	const double Heats[30] = {30, 33, 36, 34, 38, 41, 39, 37, 44, 52, 49, 45, 42, 40, 38, 43, 47, 45, 41, 39, 37, 41, 44, 58, 54, 49, 46, 43, 42, 41};
	for (int I = 29; I >= 0; --I)
	{
		kast::StreamLog Lg;
		Lg.Start = World - static_cast<double>(30 - I) * ss::net::MinutesPerDay;
		Lg.Minutes = 360.0;
		Lg.Avg = 40;
		Lg.Peak = 60;
		Lg.HeatAfter = Heats[I];
		Lg.PeakMoment = I == 9 || I == 23 ? static_cast<int>(kast::Moment::FinalTable) : -1;
		Ch.Log.insert(Ch.Log.begin(), Lg);
	}
	Ch.Goodwill = {{"pocketqueens_pk", 2}, {"freeroll_fritz", 1}, {"bubble_boy_pl", 1}};
	Ch.RaidedDay = {{"pocket_aces_pk", Today - 3}};
	for (int K = 0; K < 7; ++K)
	{
		Ch.LiveMin[static_cast<size_t>(K)] = 330;
	}
	Ch.LiveDay = Today;
	D.S.Stream.Last.PingReached = 396;
	D.S.Stream.Last.PingOf = 618;
	D.S.Stream.Last.PingParts = {253, 105, 38};
	D.S.Life.Record(World - 1500.0, copy::SponsorReadLabel("TunnelRat"), 1000, 11);
	D.S.Life.Record(World - 1440.0, "Kast payout", 5240, 6);
	D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
	D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Community);
	D.Now += 1.0;
	DrawList L = D.Shot("community");
	Keep(L);
	Time(D, "community");
	int FinalTables = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		FinalTables += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "final table" ? 1 : 0;
	}
	Check(FinalTables == 2, "community: Heat over the last 30 streams marks its two final tables");
	Check(RunIn(L, copy::ColdMeans(), D.M).Found, "community: Cold means dark");
	Check(RunIn(L, "396 of 618 followers", D.M).Found && RunIn(L, copy::PingPart(0) + " 41%", D.M).Found, "community: the Ping's reach in three parts");
	Check(RunIn(L, copy::RaidsOwed(3), D.M).Found && RunIn(L, copy::RaidsOwedShort(2), D.M).Found, "community: 3 channels owe you a raid");
	const bool Rule = RunIn(L, copy::RaidedAgo("pocket_aces_pk", 3, ss::net::WeekdayName(Today + 4, true)), D.M).Found;
	Check(Rule, "community: the 7-day rule");
	if (!Rule)
	{
		std::printf("  wanted \"%s\"\n", copy::RaidedAgo("pocket_aces_pk", 3, ss::net::WeekdayName(Today + 4, true)).c_str());
		DumpIn(L, {800.0f, 74.0f, 780.0f, 200.0f}, "the Raids card");
	}
	Check(RunIn(L, copy::StreakRow(6), D.M).Found, "community: the streak");
	const double H7 = ss::kast::growth::H7(Ch.LiveMin, Ch.LiveDay, Today);
	Check(H7 >= 24.0 ? RunIn(L, copy::StaminaLine(static_cast<int>(std::floor(H7))), D.M).Found : !RunIn(L, copy::StaminaTitle(), D.M).Found, "community: Stamina from 24 hours live in 7 days");
	Check(RunIn(L, copy::SponsorReadLabel("TunnelRat"), D.M).Found && RunIn(L, "Kast payout", D.M).Found && RunIn(L, copy::SponsorApart(), D.M).Found, "community: payouts and sponsor pay apart");
	Check(RunIn(L, "Next: Partner", D.M).Found && RunIn(L, copy::PartnerAverage(), D.M).Found, "community: Partner's road and its own-viewers average");
	// Partner's average is the core's own goal row, the number Partner unlocks on (C8: the channel's own viewers, raiders and the
	// Broadcast out; 30 here against 40 with everyone counted, once the Room's rule is in).
	for (kast::StreamLog& Lg : Ch.Log)
	{
		Lg.AvgOwn = 30;
	}
	double CoreAvg = -1.0;
	for (size_t I = 0; I < kast::Stages().size(); ++I)
	{
		if (kast::Stages()[I].Tier == 2 && kast::Stages()[I].Streams == 0)
		{
			for (const kast::Goal& G : kast::StageGoals(Ch, World, static_cast<int>(I)))
			{
				CoreAvg = G.Label.rfind("Average viewers", 0) == 0 ? G.Have : CoreAvg;
			}
		}
	}
	L = D.Frame();
	Check(CoreAvg >= 0.0 && RunIn(L, ss::Fixed(CoreAvg, 1) + " / 75", D.M).Found && (CoreAvg > 39.9 || !RunIn(L, "40.0 / 75", D.M).Found),
		"community: Partner's average is the core's own (the channel's own viewers)");
	// Before Affiliate: the real bar (C8), and what waits for the second rent.
	Ch.Affiliate = false;
	Ch.PendingCents = 13400;
	L = D.Frame();
	Check(RunIn(L, copy::AffiliateBar(25, 240, 4, 3), D.M).Found, "community: the Affiliate bar at 25 / 240 / 4 / 3");
	Check(RunIn(L, copy::PendingNote(), D.M).Found, "community: pending, paid after the next rent");
	// The schedule stays where riverline_ui presses it.
	Check(RunIn(L, "Mon", D.M, {40.0f, 744.0f, 60.0f, 38.0f}).Found, "community: the schedule's days where they were");
	// The level strip, where Stamina goes from 24 world hours live in 7 days: Kast's reach from the level and the streak
	// (Channel::Discoverability).
	for (size_t K = 0; K < Ch.LiveMin.size(); ++K)
	{
		Ch.LiveMin[K] = 0;
	}
	L = D.Frame();
	const int More = static_cast<int>(std::round((Ch.Discoverability() - 1.0) * 100.0));
	Check(More > 0 && RunIn(L, "Kast shows you to " + std::to_string(More) + "% more people browsing Poker", D.M).Found, "community: Kast shows you to N% more people browsing Poker");
	// Level 1 with no streak: nothing extra yet, so the strip says how to get it, never "0% more".
	Ch.Xp = 0;
	Ch.Streak = 0;
	L = D.Frame();
	Check(Ch.Discoverability() == 1.0 && RunIn(L, "Levels and streaks get you shown to more people browsing Poker", D.M).Found && !RunIn(L, "% more people browsing Poker", D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found,
		"community: at level 1 with no streak, how to be shown more (never 0% more)");
}

// ------------------------------------------------------------------ the Channel page's checklist (C8)

void ChannelRules()
{
	KDesk D("kast-ui-channel");
	D.S.Channel.Followers = 18;
	D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
	D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Channel);
	D.Now += 1.0;
	DrawList L = D.Shot("channel");
	Keep(L);
	Time(D, "channel");
	Check(RunIn(L, "18 / 25", D.M).Found, "channel: Affiliate's followers at the real bar (25)");
}

// ------------------------------------------------------------------ the copy

void Copy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	for (const std::string& S : kw::KastUiInventory())
	{
		Lines.push_back({"kast-ui", S});
	}
	Failures += copylint::Run("kast-ui", Lines, copylint::LetterBody);
	++Checks;
	// Every run the Kast pages drew here, and the inventory: no real brand (both tiers), nothing that says AI, bot(s),
	// autoplay or autopilot (U1), no flame word (Heat is a thermometer; Ignite owns the Flame).
	std::vector<std::string> All = AllTexts;
	for (const auto& L : Lines)
	{
		All.push_back(L.second);
	}
	std::set<std::string> Seen;
	int Bad = 0;
	for (const std::string& T : All)
	{
		if (!Seen.insert(T).second)
		{
			continue;
		}
		const char* Hit = trademarks::Find(T, trademarks::kSourceScan);
		Hit = Hit ? Hit : trademarks::Find(T, trademarks::kCopyScan);
		std::vector<std::string> Words(1);
		for (const char Ch : T)
		{
			if (std::isalnum(static_cast<unsigned char>(Ch)))
			{
				Words.back().push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(Ch))));
			}
			else if (!Words.back().empty())
			{
				Words.emplace_back();
			}
		}
		for (const std::string& W : Words)
		{
			for (const char* Ban : {"ai", "bot", "bots", "autoplay", "autopilot", "flame", "flames", "fire", "spark", "sparks", "blaze", "burn", "burning"})
			{
				Hit = Hit ? Hit : (W == Ban ? Ban : nullptr);
			}
		}
		if (Hit)
		{
			++Bad;
			std::printf("  kast-ui copy: \"%s\" holds \"%s\"\n", T.c_str(), Hit);
		}
	}
	Check(Bad == 0, "copy: no real brand, no AI or autoplay word, no flame on any Kast page");
}

// ------------------------------------------------------------------ the review's edges (review-ui-kast)

/** The top four sources, the alert's place with Hold or Crack alone, the prestige diamonds. */
void EdgeStudio()
{
	KDesk D("kast-ui-edge-studio");
	Check(D.Live(), "edge studio: live");
	D.Studio();
	// The VIEWERS tile shows the top four sources, biggest first; a fifth, smaller one waits (K8).
	kast::View V = MidMtt();
	V.Sources = {};
	V.Sources[static_cast<size_t>(kast::View::Source::Members)] = 5;
	V.Sources[static_cast<size_t>(kast::View::Source::Directory)] = 4;
	V.Sources[static_cast<size_t>(kast::View::Source::Raid)] = 3;
	V.Sources[static_cast<size_t>(kast::View::Source::Video)] = 2;
	V.Sources[static_cast<size_t>(kast::View::Source::Clip)] = 1;
	D.S.Stream.DebugView(V);
	D.Now += 1.0;
	DrawList L = D.Frame();
	Keep(L);
	Check(RunIn(L, copy::SourceCount(kast::View::Source::Members, 5), D.M, StudioTile0, true).Found && !RunIn(L, copy::SourceCount(kast::View::Source::Clip, 1), D.M, StudioTile0, true).Found,
		"edge: the VIEWERS tile's top four, the smallest source left out");
	// "M chatting" never reads above the count shown while it steps up (C12: never inverted, any frame).
	{
		kast::View Cv = MidMtt();
		Cv.Count = 4;
		Cv.ShownCount = 4;
		Cv.Chatting = 4;
		D.S.Stream.DebugView(Cv);
		D.Now += 1.0;
		D.Frame();
		D.Now += 1.0;
		D.Frame();
		Cv.Count = 20;
		Cv.Chatting = 9;
		D.S.Stream.DebugView(Cv);
		L = D.Frame();
		Check(!RunIn(L, copy::Chatting(9), D.M, StudioChat, true).Found && !RunIn(L, copy::Chatting(9), D.M, StudioTile0).Found, "edge: chatting never above the count shown as it steps up");
		D.Now += 0.5;
		L = D.Frame();
		Check(RunIn(L, copy::Chatting(9), D.M, StudioChat, true).Found, "edge: chatting reads 9 once the count lands on 20");
	}
	// Hold or Crack open without Event Mode: the alert still drops below its bar; with neither up it sits at the top.
	auto RaidTop = [&](bool HoC) {
		kast::View Hv = MidMtt();
		Hv.HoC.Open = HoC;
		Hv.HoC.Holds = 3;
		Hv.HoC.Cracks = 1;
		Hv.HoC.Equity = 0.55;
		Hv.Action = HoC ? kast::View::Slot::HoldCrack : kast::View::Slot::None;
		D.S.Stream.DebugView(Hv);
		kast::Alert A;
		A.Kind = kast::AlertKind::Raid;
		A.Who = "pocketqueens_pk";
		A.Count = 8;
		A.At = D.Now - 1.0;
		D.S.Stream.Alerts.insert(D.S.Stream.Alerts.begin(), A);
		const DrawList Ld = D.Frame();
		const KRun Raid = RunIn(Ld, "RAID!", D.M, StudioPrev);
		return Raid.Found ? Raid.Base - Raid.Size : -1.0f;
	};
	const float WithHoc = RaidTop(true);
	const float Without = RaidTop(false);
	Check(WithHoc > StudioPrev.Y + 84.0f, "edge: with Hold or Crack open (no Event Mode) the alert clears its bar");
	Check(Without > 0.0f && Without < StudioPrev.Y + 84.0f, "edge: with neither bar up the alert sits at the top");
	// One to four prestige diamonds by P: 1 under 1, 2 under 2, 3 under 3, else 4 (the $11 Big Stack's 1.40 shows two).
	Check(kw::KwDiamonds(0.5) == 1 && kw::KwDiamonds(1.0) == 1 && kw::KwDiamonds(1.40) == 2 && kw::KwDiamonds(2.0) == 2 && kw::KwDiamonds(2.6) == 3 && kw::KwDiamonds(3.2) == 4 &&
			  kw::KwDiamonds(9.0) == 4,
		"edge: the prestige diamonds by P");
}

/** The end card: Heat's move ends at the channel's Heat, "best ever" against every older night, the legend measured as drawn. */
void EdgeCard()
{
	KDesk D("kast-ui-edge-card");
	ss::kast::Channel& Ch = D.S.Channel;
	Ch.Followers = 462;
	Ch.Affiliate = true;
	Ch.Streams = 74;
	kast::Summary S = BigNight();
	for (size_t K = 0; K < S.Sources.size(); ++K)
	{
		S.Sources[K] = 30 - static_cast<int>(K); // all eleven sources, so the legend wraps
	}
	D.S.Stream.Last = S;
	D.S.StreamCard = true;
	D.Studio();
	const double Start = D.S.Stream.StartWorld;
	// Tonight's row (60+ world minutes) holds the committed Heat: the commit, a raid's bump and a record peak, 76 (Sun run).
	kast::StreamLog Tonight;
	Tonight.Start = Start;
	Tonight.Minutes = S.Minutes;
	Tonight.Peak = S.Peak;
	Tonight.Follows = S.Follows;
	Tonight.HeatAfter = 76.0;
	kast::StreamLog Big;
	Big.Start = Start - 2.0 * ss::net::MinutesPerDay;
	Big.Minutes = 300.0;
	Big.Peak = 150;
	Big.Follows = 12;
	Big.HeatAfter = 40.0;
	kast::StreamLog Small = Big;
	Small.Start = Start - 4.0 * ss::net::MinutesPerDay;
	Small.Peak = 50;
	Ch.Log = {Tonight, Big, Small};
	DrawList L = D.Frame();
	Keep(L);
	Check(RunIn(L, copy::HeatMove(2, 4, 40.0), D.M).Found && !RunIn(L, copy::HeatMove(2, 3, 29.0), D.M).Found, "edge: Heat's move ends where the channel's Heat stands (Sun run, +40)");
	Check(!RunIn(L, "your best room ever", D.M).Found && RunIn(L, "best night ever", D.M).Found, "edge: best ever is against every older night (an older peak of 150 stands)");
	// The legend wraps; the clip line still sits under the body's last row (the card measures what it draws).
	const KRun Goal = RunIn(L, copy::NextGoal(38, 500), D.M);
	const KRun Clip = RunIn(L, copy::ClipOfTheNight(S.BestClip, true), D.M);
	Check(Goal.Found && Clip.Found && Clip.Base - Goal.Base >= 28.0f, "edge: the clip line sits under the next goal (no overlap after a wrapped legend)");
	if (!(Goal.Found && Clip.Found && Clip.Base - Goal.Base >= 28.0f))
	{
		std::printf("  goal at %.1f, clip at %.1f\n", Goal.Base, Clip.Base);
	}
	// A night under 60 world minutes writes no row: the newest row is an older night, and it counts; Heat moves by the commit.
	Ch.Log = {Big, Small};
	L = D.Frame();
	Check(!RunIn(L, "your best room ever", D.M).Found, "edge: a short night isn't the best room ever under an older 150");
	Check(RunIn(L, copy::HeatMove(2, 3, 29.0), D.M).Found, "edge: a short night's Heat moves by the commit");
	// One troll is one troll; with mods the line ends there.
	D.S.Stream.Last.Trolls = 1;
	Ch.Mods.clear();
	L = D.Frame();
	Keep(L);
	Check(RunIn(L, "1 troll showed up: a couple of mods would help.", D.M).Found && !RunIn(L, "1 trolls", D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found, "edge: 1 troll showed up, not 1 trolls");
	Ch.Mods.push_back(kast::Moderator{});
	Ch.Mods.back().Name = "nitbox";
	D.S.Stream.Last.Trolls = 3;
	L = D.Frame();
	Check(RunIn(L, "3 trolls showed up.", D.M).Found, "edge: 3 trolls showed up. (with a mod)");
}

/** TONIGHT names a tipper only while one tipped; the sponsor read is offered only when it can pay (C13). */
void EdgeMoney()
{
	KDesk D("kast-ui-edge-money");
	const double Signed = D.S.WorldMinutes();
	const std::string Offer = ss::sponsor::MakeOffer(D.S.Life.Sponsors, D.S.Life, ss::rep::Public(), "overclock", Signed, "kast-ui", true);
	Check(ss::sponsor::Sign(D.S.Life.Sponsors, D.S.Life, D.S.BankrollCents, Offer, Signed).empty(), "edge money: Overclock signed");
	Check(D.Live(), "edge money: live");
	D.Studio();
	kast::View V = MidMtt();
	D.S.Stream.DebugView(V);
	D.S.Stream.Tonight.TipCents = 1000;
	kast::Alert T1;
	T1.Kind = kast::AlertKind::Tip;
	T1.Who = "dee";
	T1.Cents = 500;
	kast::Alert T2 = T1;
	T2.Who = "mei";
	D.S.Stream.Feed.insert(D.S.Stream.Feed.begin(), T1);
	D.S.Stream.Feed.insert(D.S.Stream.Feed.begin(), T2);
	D.Now += 1.0;
	DrawList L = D.Frame();
	Keep(L);
	Check(!RunIn(L, copy::Tipped("mei", 1000), D.M).Found && !RunIn(L, copy::Tipped("dee", 1000), D.M).Found, "edge: TONIGHT never puts two people's tips under one name");
	D.S.Stream.Feed.erase(D.S.Stream.Feed.begin());
	D.S.Stream.Tonight.TipCents = 500;
	L = D.Frame();
	Check(RunIn(L, copy::Tipped("dee", 500), D.M).Found, "edge: one tipper tonight (dee tipped $5)");
	// The read: shown before 30 world minutes but off; on after 30 with someone watching; off at 0 watching; never offered
	// past the period's cap (1.5x the deal's reads ask).
	auto Read = [&]() {
		const DrawList Ld = D.Frame();
		return RunIn(Ld, "Read for ", D.M, {20.0f, 700.0f, 1008.0f, 80.0f}, true);
	};
	KRun R0 = Read();
	Check(R0.Found && SameColour(R0.Col, kw::KDim), "edge: the read waits for 30 world minutes live");
	D.S.Stream.StartWorld -= 40.0;
	D.S.Stream.DebugView(V);
	KRun R1 = Read();
	Check(R1.Found && !SameColour(R1.Col, kw::KDim), "edge: after 30 world minutes with someone watching, the read is on");
	V.Count = 0;
	V.ShownCount = 0;
	V.Chatting = 0;
	D.S.Stream.DebugView(V);
	KRun R2 = Read();
	Check(R2.Found && SameColour(R2.Col, kw::KDim), "edge: nobody watching, the read is off");
	V = MidMtt();
	D.S.Stream.DebugView(V);
	for (ss::sponsor::Contract& Ct : D.S.Life.Sponsors.Contracts)
	{
		Ct.Now.Reads = Ct.Brand == "overclock" ? 99 : Ct.Now.Reads;
	}
	KRun R3 = Read();
	Check(!R3.Found, "edge: past the period's reads cap the read isn't offered (it would pay nothing)");
}

/** A member Nodded tonight can't take a second Nod; one here and not yet Nodded can. */
void EdgeNod()
{
	KDesk D("kast-ui-edge-nod");
	Check(D.Live(), "edge nod: live");
	D.Studio();
	kast::Member Nb;
	Nb.Name = "nitbox";
	D.S.Channel.Members.push_back(Nb);
	kast::View V = MidMtt();
	V.MemberFlags.assign(D.S.Channel.Members.size(), 8);
	V.MemberFlags.back() = 8 | 4;
	D.S.Stream.DebugView(V);
	Chat(D, {{"nitbox", "seat 7 plays every hand"}});
	D.Now += 1.0;
	DrawList L = D.Frame();
	ClickRun(D, RunIn(L, "nitbox:", D.M, StudioChat));
	L = D.Frame();
	Keep(L);
	const KRun Off = RunIn(L, copy::NodButton(), D.M, StudioChat);
	Check(Off.Found && SameColour(Off.Col, kw::KDim), "edge: a member Nodded tonight can't take a second");
	V.MemberFlags.back() = 8;
	D.S.Stream.DebugView(V);
	L = D.Frame();
	const KRun On = RunIn(L, copy::NodButton(), D.M, StudioChat);
	Check(On.Found && !SameColour(On.Col, kw::KDim), "edge: a member here and not yet Nodded can");
}

// ------------------------------------------------------------------ the C-K gate's fixes (fix-ui): M4, L1, L11, L5

/** C-K's M4 (the lead's ruling): Heat's ghost has no number in a stream's first 15 world minutes, nor until the forecast has a fair
 *  read (tonight's running average inside or above the band with someone watching, or 60 world minutes live); then the signed
 *  number, a negative one too. No ghost fill while the night settles. */
void FixGhost()
{
	KDesk D("kast-ui-fix-ghost");
	Check(D.Live(), "ghost: live");
	D.Studio();
	const Rect Tile4{660.0f, 772.0f, 170.0f, 174.0f};
	// The real Room, a few real seconds in: the ghost it computes opens below 0 (its running average starts at 0).
	D.Now = Step(D.S, D.Now, 6.0);
	DrawList L = D.Frame();
	const double Raw = D.S.Stream.GetView().GhostGain;
	Check(D.S.Stream.GetView().Heat > 0.0 && RunIn(L, copy::GhostSettling(), D.M, Tile4).Found && !RunIn(L, "tonight -", D.M, Tile4, true).Found &&
			  !RunIn(L, "tonight +", D.M, Tile4, true).Found,
		"ghost: the real Room's first minutes read tonight: settling in, no number");
	std::printf("  ghost: the Room's own ghost reads %.1f at world minute %.2f (held back as settling in)\n", Raw, D.S.Stream.Uptime(D.S.WorldMinutes()));
	// The rule, on views put up at a world minute, with tonight's graph (one sample a world minute) as the running average.
	kast::View V = MidMtt(); // forecast 8-14
	auto At = [&](double Minutes, float Each, double Gain, const kast::growth::Band& Fc) {
		V.GhostGain = Gain;
		V.Fc = Fc;
		D.S.Stream.DebugView(V);
		D.S.Stream.StartWorld = D.S.WorldMinutes() - Minutes;
		D.S.Stream.Graph.assign(static_cast<size_t>(std::floor(Minutes)) + 1, Each);
		D.Now += 0.5;
		return D.Frame();
	};
	auto Settling = [&](const DrawList& Ld) { return RunIn(Ld, copy::GhostSettling(), D.M, Tile4).Found && !RunIn(Ld, "so far", D.M, Tile4, true).Found; };
	const kast::growth::Band Band{8.0, 11.0, 14.0};
	L = At(14.9, 12.0f, 3.0, Band);
	kw::KastHeat H = kw::KastHeatNow(D.S, D.S.WorldMinutes());
	Check(Settling(L) && !H.Settled && H.Ghost == 0.0, "ghost: 14.9 world minutes in, inside the forecast: still settling in, no ghost fill");
	L = At(15.5, 12.0f, 3.0, Band);
	H = kw::KastHeatNow(D.S, D.S.WorldMinutes());
	Check(RunIn(L, copy::GhostTonight(3.0), D.M, Tile4).Found && H.Settled && H.Ghost == 3.0, "ghost: past 15 world minutes, the average inside the forecast: tonight +3 so far");
	L = At(25.0, 5.0f, 12.0, Band);
	H = kw::KastHeatNow(D.S, D.S.WorldMinutes());
	Check(Settling(L) && H.Ghost == 0.0, "ghost: under the forecast's band at 25 world minutes: no number, no fill (even a positive one: news on a slow start)");
	L = At(59.5, 5.0f, -6.0, Band);
	Check(Settling(L), "ghost: still under the band at 59 world minutes: settling in");
	L = At(61.0, 5.0f, -6.0, Band);
	Check(RunIn(L, copy::GhostTonight(-6.0), D.M, Tile4).Found, "ghost: from 60 world minutes the number stands, a negative one too (tonight -6 so far)");
	L = At(30.0, 0.0f, -9.0, {0.0, 1.0, 2.0});
	Check(Settling(L), "ghost: a band from 0 and nobody watching yet: no -9");
	L = At(30.0, 1.5f, -2.0, {0.0, 1.0, 2.0});
	Check(RunIn(L, copy::GhostTonight(-2.0), D.M, Tile4).Found, "ghost: a band from 0 with someone watching: the number");
	L = At(12.0, 3.0f, 4.0, {0.0, 0.0, 0.0});
	Check(Settling(L), "ghost: no forecast band, 12 world minutes in: settling in");
	L = At(16.0, 3.0f, 4.0, {0.0, 0.0, 0.0});
	Check(RunIn(L, copy::GhostTonight(4.0), D.M, Tile4).Found, "ghost: no forecast band: the number from 15 world minutes");
	// review-fix-ui: the running average as it stood at each world minute (the graph's prefix). Once it has read inside the band at
	// 15 or later the night stays settled: an early raid that lifted it in and then left doesn't turn the number back into words.
	auto AtGraph = [&](double Minutes, const std::vector<float>& G, double Gain) {
		V.GhostGain = Gain;
		V.Fc = Band;
		D.S.Stream.DebugView(V);
		D.S.Stream.StartWorld = D.S.WorldMinutes() - Minutes;
		D.S.Stream.Graph = G;
		D.Now += 0.5;
		return D.Frame();
	};
	std::vector<float> Raided(16, 12.0f); // 12 watching through minute 15: an average of 12 at 15, inside 8-14
	Raided.resize(26, 0.0f);               // then nobody: 192 / 26 = 7.4 at minute 25, under the band
	L = AtGraph(25.5, Raided, -1.0);
	Check(RunIn(L, copy::GhostTonight(-1.0), D.M, Tile4).Found, "ghost: settled at 15, the number stays when the average dips under the band (tonight -1 so far)");
	std::vector<float> Early(3, 30.0f); // in the band only in minutes 0-2: 90 / 16 = 5.6 at 15, 4.3 at 20
	Early.resize(21, 0.0f);
	L = AtGraph(20.5, Early, -3.0);
	Check(Settling(L), "ghost: an average in the band only in the first 15 world minutes doesn't settle the night");
	L = At(20.0, 8.0f, -2.0, Band);
	Check(RunIn(L, copy::GhostTonight(-2.0), D.M, Tile4).Found, "ghost: an average at the band's bottom (8 of 8-14) is a fair read: tonight -2 so far");
	// The header chip and StreamSide's thermometer read the same Heat: no ghost while settling (KastHeatNow is theirs too).
	L = At(5.0, 12.0f, 25.0, Band);
	H = kw::KastHeatNow(D.S, D.S.WorldMinutes());
	Check(H.Live && !H.Settled && H.Ghost == 0.0 && kw::KastGhostLine(H) == copy::GhostSettling(), "ghost: every surface's Heat holds no ghost while the night settles");
}

/** C-K's L1: the site's Broadcast reads THE BROADCAST (the three site voices' lines at the alert's size); every other raid RAID!. */
void FixHeadlines()
{
	KDesk D("kast-ui-fix-headline");
	Check(D.Live(), "headline: live");
	D.Studio();
	D.S.Stream.DebugView(MidMtt());
	struct Case
	{
		std::string Who;
		int Count;
		std::string Text;
		bool Broadcast;
		const char* What;
	};
	const Case Cases[] = {
		{"RiverLine Live", 33, copy::Broadcast(static_cast<int>(ss::net::SiteId::RiverLine), 33), true, "headline: RiverLine Live's Broadcast reads THE BROADCAST"},
		{"Ignite's Bandstand", 41, copy::Broadcast(static_cast<int>(ss::net::SiteId::Ignite), 41), true, "headline: Ignite's Bandstand reads THE BROADCAST"},
		{"RunItTwice's Newsdesk", 1250, copy::Broadcast(static_cast<int>(ss::net::SiteId::RunItTwice), 1250), true, "headline: RunItTwice's Newsdesk (1,250) reads THE BROADCAST"},
		{"kat_v", 14, std::string(), false, "headline: a channel's raid reads RAID!"},
		{"pocketqueens_pk", 8, copy::WelcomeRaid("pocketqueens_pk", 8), false, "headline: the welcome raid reads RAID!"},
		{"RiverLine Live", 34, copy::Broadcast(0, 33), false, "headline: a line that isn't the Broadcast's for the alert's size reads RAID!"},
	};
	for (const Case& Cs : Cases)
	{
		kast::Alert A;
		A.Kind = kast::AlertKind::Raid;
		A.Who = Cs.Who;
		A.Count = Cs.Count;
		A.Text = Cs.Text;
		D.Now += 6.0;
		A.At = D.Now - 0.5;
		D.S.Stream.Alerts.insert(D.S.Stream.Alerts.begin(), A);
		const DrawList L = D.Frame();
		const bool Head = RunIn(L, kw::KastBroadcastHeadline(), D.M, StudioPrev).Found;
		const bool Raid = RunIn(L, "RAID!", D.M, StudioPrev).Found;
		Check(kw::KastIsBroadcast(A) == Cs.Broadcast && Head == Cs.Broadcast && Raid == !Cs.Broadcast, Cs.What);
	}
}

/** C-K's L11 (the UI half, on fix-room's appended View fields) and L5 (KastCopy's accessors where the UI drew its own words). */
void FixL11()
{
	// The VIEWERS tile at a true 0: nobody here, and the directory's impressions ("41 saw you in the directory").
	{
		KDesk D("kast-ui-fix-seen");
		Check(D.Live(), "seen: live");
		D.Studio();
		kast::View V;
		V.Heat = 27.0;
		V.Tier = 1;
		V.Impressions = 41;
		V.ImpressionsCame = 2;
		V.Fc = {1.0, 2.0, 4.0};
		D.S.Stream.DebugView(V);
		D.Frame(); // the count steps to the view's 0
		D.Now += 1.0;
		DrawList L = D.Shot("honest_zero_seen");
		Keep(L);
		const KRun Nobody = RunIn(L, copy::NobodyHere(), D.M, StudioTile0);
		const KRun Saw = RunIn(L, copy::SawYou(41), D.M, StudioTile0);
		Check(Nobody.Found && Saw.Found && Saw.Base > Nobody.Base, "seen: at a true 0 the VIEWERS tile says 41 saw you in the directory, under nobody here right now");
		V.Impressions = 0;
		D.S.Stream.DebugView(V);
		L = D.Frame();
		Check(RunIn(L, copy::NobodyHere(), D.M, StudioTile0).Found && !RunIn(L, "saw you", D.M, StudioTile0, true).Found, "seen: no impressions, no line");
		// WELCOME THE RAID with its window left in Studio's slot (the mockup's 1:52).
		V = MidMtt();
		V.Action = kast::View::Slot::WelcomeRaid;
		V.WelcomeRaidSecs = 112;
		D.S.Stream.DebugView(V);
		L = D.Frame();
		Check(RunIn(L, copy::WelcomeTheRaidButton() + " 1:52", D.M, {20.0f, 730.0f, 220.0f, 36.0f}).Found, "welcome: Studio's slot counts down (WELCOME THE RAID 1:52)");
		V.WelcomeRaidSecs = 7;
		D.S.Stream.DebugView(V);
		L = D.Frame();
		Check(RunIn(L, copy::WelcomeTheRaidButton() + " 0:07", D.M, {20.0f, 730.0f, 220.0f, 36.0f}).Found, "welcome: 0:07 left");
		// A guest Nodded tonight (View::Nodded; no member row to flag) can't take a second Nod; another guest can.
		V = MidMtt();
		V.Nodded = {"guest_rio"};
		D.S.Stream.DebugView(V);
		Chat(D, {{"guest_rio", "first time here"}, {"guest_ana", "hello from Lisbon"}});
		D.Now += 1.0;
		L = D.Frame();
		ClickRun(D, RunIn(L, "guest_rio:", D.M, StudioChat));
		L = D.Frame();
		const KRun Off = RunIn(L, copy::NodButton(), D.M, StudioChat);
		Check(Off.Found && SameColour(Off.Col, kw::KDim), "nod: a guest Nodded tonight can't take a second");
		D.Click(600.0f, 768.0f); // a press elsewhere (between the controls and the tiles) closes the menu
		L = D.Frame();
		ClickRun(D, RunIn(L, "guest_ana:", D.M, StudioChat));
		L = D.Frame();
		const KRun On = RunIn(L, copy::NodButton(), D.M, StudioChat);
		Check(On.Found && !SameColour(On.Col, kw::KDim), "nod: a guest not Nodded tonight can");
		if (!On.Found || SameColour(On.Col, kw::KDim))
		{
			DumpIn(L, StudioChat, "the chat with guest_ana's menu");
		}
		// Browse's "N channels live": every Poker channel live, the network's with the directory's, yours with them (View::LiveChannels;
		// before the Room fills it, kast::ChannelsLive and yours).
		D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
		L = D.Frame();
		const int Off0 = kast::ChannelsLive(D.S.WorldMinutes()) + 1;
		const bool Before = RunIn(L, copy::Number(Off0) + " channels live", D.M, {0.0f, 120.0f, 800.0f, 30.0f}, true).Found;
		Check(Before && RunIn(L, "Poker", D.M, {0.0f, 80.0f, 400.0f, 40.0f}).Found, "browse: N channels live counts the network and you (before the Room's count), under the Poker heading");
		if (!Before)
		{
			std::printf("  browse: wanted %d channels live\n", Off0);
			DumpIn(L, {0.0f, 80.0f, 1600.0f, 80.0f}, "Browse's head");
		}
		V.LiveChannels = 57;
		D.S.Stream.DebugView(V);
		L = D.Frame();
		Check(RunIn(L, "57 channels live", D.M, {0.0f, 120.0f, 800.0f, 30.0f}, true).Found, "browse: N channels live is the Room's count (View::LiveChannels)");
	}
	// Top clips: a clip that went into the Clip Lane says so; the other doesn't.
	{
		KDesk D("kast-ui-fix-lane");
		kast::Clip A;
		A.Title = "river nine";
		A.By = "kat_v";
		A.Views = 1840;
		A.Lane = true;
		kast::Clip B;
		B.Title = "the hero call";
		B.By = "nitbox";
		B.Views = 310;
		D.S.Channel.Clips = {A, B};
		D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
		D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Channel);
		D.Now += 1.0;
		DrawList L = D.Shot("channel_clips");
		Keep(L);
		const std::string Tag = "\xC2\xB7 " + kw::KastClipLaneTag();
		int Tags = 0;
		float TagY = 0.0f;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Tag)
			{
				++Tags;
				TagY = Cmd.Text.BaselineY;
			}
		}
		const KRun Views = RunIn(L, ss::ui::rlkast_detail::KastCount(1840) + " views", D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true);
		Check(kw::KastClipLaneTag() == "in the Kast feed" && Tags == 1 && Views.Found && std::fabs(TagY - Views.Base) < 0.5f, "clips: the Clip Lane's clip says in the Kast feed, the other doesn't");
		// review-fix-ui: Browse off air, every Poker channel live and every viewer: the directory's and the network's.
		D.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
		D.Now += 1.0;
		L = D.Frame();
		const double World = D.S.WorldMinutes();
		long long Viewers = 0;
		for (const kast::Streamer& St : kast::Directory())
		{
			Viewers += std::max(0, kast::ViewersNow(St, World));
		}
		long long Net = 0;
		for (const kast::SmallChannel& Sc : kast::Network(World))
		{
			Net += Sc.Viewers;
		}
		const std::string Head = copy::Number(kast::ChannelsLive(World)) + " channels live \xC2\xB7 " + copy::Number(static_cast<int>(Viewers + Net)) + " viewers";
		Check(Net > 0 && RunIn(L, Head, D.M, {0.0f, 120.0f, 800.0f, 30.0f}, true).Found, "browse: off air, N channels live and the viewers of all of them (the network's too)");
		if (!RunIn(L, Head, D.M, {0.0f, 120.0f, 800.0f, 30.0f}, true).Found)
		{
			std::printf("  browse: wanted %s\n", Head.c_str());
			DumpIn(L, {0.0f, 80.0f, 1600.0f, 80.0f}, "Browse's head");
		}
	}
}

/** C-K's L5 (the UI half): the FOLLOWERS tile's foot is copy::AffiliateToGo; the end card's NEW FOLLOWERS is NewFollowersLine; the
 *  card draws the night's firsts (Summary::Firsts, M3) and the Room's own why line (Summary::Why) when it gives them. */
void FixCopy()
{
	KDesk D("kast-ui-fix-copy");
	D.S.Channel.Followers = 18;
	int Goal = 0;
	Check(kw::KastFollowerFoot(D.S.Channel, Goal) == copy::AffiliateToGo(7, 25) && Goal == 25, "copy: the FOLLOWERS tile's foot is KastCopy's (7 to go: Affiliate at 25)");
	D.S.Channel.Followers = 462;
	D.S.Channel.Affiliate = true;
	D.S.Channel.Streams = 74;
	kast::Summary S = BigNight();
	S.Firsts = {copy::FirstStrangerTip("lucia.mm", 200), copy::FirstRaidIn("freeroll_fritz", 3)};
	S.Why = {copy::WhyRun(kast::growth::RunStage::FinalTable, 980, 0, "Tuesday"), std::string()};
	D.S.Stream.Last = S;
	D.S.StreamCard = true;
	D.Studio();
	DrawList L = D.Shot("end_card_firsts");
	Keep(L);
	Check(RunIn(L, copy::FirstStrangerTip("lucia.mm", 200), D.M).Found && RunIn(L, copy::FirstRaidIn("freeroll_fritz", 3), D.M).Found, "copy: the end card draws the night's firsts (why their sting played)");
	Check(RunIn(L, S.Why[0], D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found, "copy: the end card's Heat line is the Room's own why (Summary::Why)");
	const std::string Fl = copy::NewFollowersLine(S.Follows, false);
	Check(RunIn(L, Fl, D.M).Found, "copy: NEW FOLLOWERS is KastCopy's line (+N)");
	Check(Overflows(L, {440.0f, 62.0f, 720.0f, 900.0f}, D.M, "end card (firsts)") == 0, "copy: the card with its firsts fits");
	// A night under 60 world minutes commits no Heat and reads no forecast (§7): the Room's fallback "Steady: nights near your
	// forecast" never sits beside the card's UNDER YOUR FORECAST (riverline_ui's kast_summary, a 9-minute night); from 60 on the
	// Room's line is the card's.
	kast::Summary Sh = S;
	Sh.Minutes = 9.42;
	Sh.Avg = 11;
	Sh.Fc = {13.0, 17.5, 22.0};
	Sh.Why = {copy::WhySteady(), std::string()};
	D.S.Stream.Last = Sh;
	D.Studio();
	DrawList Ls = D.Frame();
	Check(RunIn(Ls, "UNDER YOUR FORECAST", D.M).Found && !RunIn(Ls, copy::WhySteady(), D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found,
		"copy: a night under 60 minutes keeps the channel's own why (no Steady beside UNDER YOUR FORECAST)");
	Sh.Minutes = 60.0;
	Sh.Why = {copy::WhyUnderForecast(37), std::string()};
	D.S.Stream.Last = Sh;
	DrawList L60 = D.Frame();
	Check(RunIn(L60, copy::WhyUnderForecast(37), D.M, {0.0f, 0.0f, 0.0f, 0.0f}, true).Found, "copy: a night of 60 minutes draws the Room's why (Summary::Why)");
}

/** The stream preview's capture: the client captured 15 times a real second and copied in between; the same instant, a clock
 *  that ran backwards or a new key re-captures at once; a copied-in frame draws exactly what a fresh capture draws. */
void EdgeCapture()
{
	// The mechanism, on a canvas of its own (a laptop key no RiverLine can share: a function-local static's address).
	{
		static int KcFakeLaptop = 0;
		const auto* Laptop = reinterpret_cast<const ss::ui::RiverLine*>(&KcFakeLaptop);
		const TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		int Draws = 0;
		auto Paint = [&] {
			++Draws;
			C.FillRect({30.0f, 80.0f, 100.0f, 40.0f}, ss::ui::Hex(0x336699));
			C.Text("the client", 40.0f, 100.0f, ss::ui::Ts(14.0f));
			C.FillRoundRect({140.0f, 80.0f, 60.0f, 30.0f}, 6.0f, ss::ui::Hex(0x993366));
		};
		kw::KastCaptureKey K;
		K.Frame = StudioPrev;
		K.Screen = static_cast<int>(ss::Screen::Table); // at a table: 15 captures a second
		C.FillRect({0.0f, 0.0f, 10.0f, 10.0f}, ss::ui::Hex(0x111111)); // an open triangle run the client's first shape continues
		auto Added = [&](size_t V0, size_t I0, size_t C0) {
			std::vector<std::string> Out;
			Out.push_back("v" + std::to_string(L.Vertices.size() - V0));
			// Every index added, as the vertex it names (the copy lands at another base and may continue another run).
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles)
				{
					continue;
				}
				for (size_t P = Cmd.IndexStart; P < static_cast<size_t>(Cmd.IndexStart) + Cmd.IndexCount; ++P)
				{
					if (P >= I0)
					{
						const ss::ui::Vertex& Vx = L.Vertices[Cmd.VertexStart + L.Indices[P]];
						Out.push_back(std::to_string(Vx.X) + "," + std::to_string(Vx.Y) + "," + std::to_string(Vx.Col.R) + "," + std::to_string(Vx.Col.A));
					}
				}
			}
			for (size_t I = C0; I < L.Cmds.size(); ++I)
			{
				Out.push_back(L.Cmds[I].Type == ss::ui::DrawCmd::Kind::Text ? "t" + L.Cmds[I].Text.Text : "c");
			}
			return Out;
		};
		size_t V0 = L.Vertices.size(), I0 = L.Indices.size(), C0 = L.Cmds.size();
		kw::KastPreviewCapture(C, Laptop, K, 5.0, Paint);
		const std::vector<std::string> Drawn = Added(V0, I0, C0);
		C.FillRect({0.0f, 0.0f, 10.0f, 10.0f}, ss::ui::Hex(0x111111));
		V0 = L.Vertices.size(), I0 = L.Indices.size(), C0 = L.Cmds.size();
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 1.0 / 30.0, Paint);
		const std::vector<std::string> Copied = Added(V0, I0, C0);
		Check(Draws == 1 && Copied == Drawn && !Drawn.empty(), "capture: inside 1/15 s the capture is copied in, vertex for vertex (an open run continued)");
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 1.0 / 30.0, Paint);
		Check(Draws == 2, "capture: a redraw at the same instant draws the client again");
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 2.0 / 30.0, Paint);
		Check(Draws == 2, "capture: 1/30 s after the capture, copied");
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 3.0 / 30.0, Paint);
		Check(Draws == 3, "capture: 1/15 s after the capture, captured again (every other draw at the laptop's 30)");
		K.Screen = static_cast<int>(ss::Screen::Lobby);
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 3.5 / 30.0, Paint);
		Check(Draws == 4, "capture: a new key (the client's screen) captures at once");
		// On the lobby, 10 a second: 1/30 and 2/30 s on it copies, 3/30 s on it captures.
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 4.5 / 30.0, Paint);
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 5.5 / 30.0, Paint);
		Check(Draws == 4, "capture: on the lobby, copied for 1/10 s");
		kw::KastPreviewCapture(C, Laptop, K, 5.0 + 6.5 / 30.0, Paint);
		Check(Draws == 5, "capture: on the lobby, captured again 1/10 s on (every third draw at the laptop's 30)");
		kw::KastPreviewCapture(C, Laptop, K, 5.0, Paint);
		Check(Draws == 6, "capture: a clock that ran backwards captures");
		kw::KastCaptureKey K2 = K;
		Check(K2.Same(K), "capture: a key is the same as itself");
		K2.Xf.Tx = 1.0f;
		Check(!K2.Same(K), "capture: the laptop's transform is in the key");
		// Every field of the key re-captures at once, 4 ms after a capture (the picture was drawn for another rect, client, screen,
		// table count, pixel scale or transform); the same key 4 ms on is copied (the clock isn't what decides here).
		struct KcField
		{
			const char* Name;
			void (*Edit)(kw::KastCaptureKey&);
		};
		const KcField Fields[] = {
			{"Frame.X", [](kw::KastCaptureKey& Kx) { Kx.Frame.X += 1.0f; }},
			{"Frame.Y", [](kw::KastCaptureKey& Kx) { Kx.Frame.Y += 1.0f; }},
			{"Frame.W", [](kw::KastCaptureKey& Kx) { Kx.Frame.W += 1.0f; }},
			{"Frame.H", [](kw::KastCaptureKey& Kx) { Kx.Frame.H += 1.0f; }},
			{"Client", [](kw::KastCaptureKey& Kx) { Kx.Client += 1; }},
			{"Screen", [](kw::KastCaptureKey& Kx) { Kx.Screen = static_cast<int>(ss::Screen::Results); }},
			{"Tables", [](kw::KastCaptureKey& Kx) { Kx.Tables += 1; }},
			{"PxScale", [](kw::KastCaptureKey& Kx) { Kx.PxScale += 0.5f; }},
			{"Xf.A", [](kw::KastCaptureKey& Kx) { Kx.Xf.A = 2.0f; }},
			{"Xf.B", [](kw::KastCaptureKey& Kx) { Kx.Xf.B = 0.5f; }},
			{"Xf.C", [](kw::KastCaptureKey& Kx) { Kx.Xf.C = 0.5f; }},
			{"Xf.D", [](kw::KastCaptureKey& Kx) { Kx.Xf.D = 2.0f; }},
			{"Xf.Tx", [](kw::KastCaptureKey& Kx) { Kx.Xf.Tx = 3.0f; }},
			{"Xf.Ty", [](kw::KastCaptureKey& Kx) { Kx.Xf.Ty = 3.0f; }},
		};
		double Tk = 20.0;
		kw::KastPreviewCapture(C, Laptop, K, Tk, Paint); // captured: the clock far past the last capture
		bool AllRecapture = true;
		bool SameCopies = true;
		for (const KcField& Fd : Fields)
		{
			kw::KastCaptureKey Kx = K;
			Fd.Edit(Kx);
			const int Before = Draws;
			Tk += 0.004;
			kw::KastPreviewCapture(C, Laptop, Kx, Tk, Paint);
			if (Draws != Before + 1)
			{
				AllRecapture = false;
				std::printf("  capture: a new %s didn't re-capture\n", Fd.Name);
			}
			Tk += 0.004;
			kw::KastPreviewCapture(C, Laptop, K, Tk, Paint); // back to the first key: captured again
			const int Back = Draws;
			Tk += 0.004;
			kw::KastPreviewCapture(C, Laptop, K, Tk, Paint);
			SameCopies = SameCopies && Back == Before + 2 && Draws == Back;
		}
		Check(AllRecapture, "capture: every field of the key (rect, client, screen, tables, pixel scale, transform) re-captures at once");
		Check(SameCopies, "capture: the same key 4 ms after a capture is copied");
	}
	// A real page: Studio live with RiverLine's lobby in the preview, settled. A frame 1/30 s on copies the capture in; drawn again
	// at that instant it re-captures. The two lists hold the same vertices, indices, commands and text runs, run for run; only what
	// the client itself animates in 1/30 s may sit or tint differently (the copy is the picture of 1/30 s before).
	KDesk D("kast-ui-capture");
	Check(D.Live(), "capture: live");
	D.Studio();
	D.Now += 1.0;
	D.Frame(); // the lobby's rows rise in over the first second it's drawn
	D.Now += 3.0;
	D.Frame();
	D.Now += 1.0 / 30.0;
	long long Cap0 = 0, Copy0 = 0, Cap1 = 0, Copy1 = 0, Cap2 = 0, Copy2 = 0;
	kw::KastCaptureCounts(&D.RL, Cap0, Copy0);
	const DrawList Copied = D.Frame();
	kw::KastCaptureCounts(&D.RL, Cap1, Copy1);
	const DrawList Fresh = D.Frame();
	kw::KastCaptureCounts(&D.RL, Cap2, Copy2);
	Check(Fresh.ToJson() == D.Frame().ToJson(), "capture: two redraws at one instant draw the same, byte for byte");
	Check(Copy1 == Copy0 + 1 && Cap1 == Cap0 && Cap2 == Cap1 + 1 && Copy2 == Copy1, "capture: 1/30 s on the frame copies the capture; drawn again at that instant it re-captures");
	bool Same = Copied.Vertices.size() == Fresh.Vertices.size() && Copied.Indices == Fresh.Indices && Copied.Cmds.size() == Fresh.Cmds.size() && Copied.Vertices.size() > 10000;
	if (!Same)
	{
		std::printf("  capture: copied %zu vertices %zu indices %zu cmds, fresh %zu / %zu / %zu\n", Copied.Vertices.size(), Copied.Indices.size(), Copied.Cmds.size(), Fresh.Vertices.size(), Fresh.Indices.size(),
			Fresh.Cmds.size());
	}
	for (size_t I = 0; Same && I < Copied.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& A = Copied.Cmds[I];
		const ss::ui::DrawCmd& B = Fresh.Cmds[I];
		Same = A.Type == B.Type && A.VertexStart == B.VertexStart && A.VertexCount == B.VertexCount && A.IndexStart == B.IndexStart && A.IndexCount == B.IndexCount &&
			   (A.Type != ss::ui::DrawCmd::Kind::Text || (A.Text.Text == B.Text.Text && A.Text.SizePx == B.Text.SizePx));
		if (!Same)
		{
			std::printf("  capture: command %zu differs: type %d/%d, v %u+%u / %u+%u, text \"%s\" / \"%s\"\n", I, static_cast<int>(A.Type), static_cast<int>(B.Type), A.VertexStart, A.VertexCount, B.VertexStart,
				B.VertexCount, A.Text.Text.c_str(), B.Text.Text.c_str());
		}
	}
	int Moved = 0;
	int Tints = 0;
	float Mx0 = 1e9f, My0 = 1e9f, Mx1 = -1e9f, My1 = -1e9f, Dmax = 0.0f;
	for (size_t I = 0; Same && I < Copied.Vertices.size(); ++I)
	{
		if (Copied.Vertices[I].X != Fresh.Vertices[I].X || Copied.Vertices[I].Y != Fresh.Vertices[I].Y)
		{
			Mx0 = std::min(Mx0, Fresh.Vertices[I].X);
			My0 = std::min(My0, Fresh.Vertices[I].Y);
			Mx1 = std::max(Mx1, Fresh.Vertices[I].X);
			My1 = std::max(My1, Fresh.Vertices[I].Y);
			Dmax = std::max(Dmax, std::fabs(Copied.Vertices[I].X - Fresh.Vertices[I].X) + std::fabs(Copied.Vertices[I].Y - Fresh.Vertices[I].Y));
		}
		Moved += Copied.Vertices[I].X != Fresh.Vertices[I].X || Copied.Vertices[I].Y != Fresh.Vertices[I].Y ? 1 : 0;
		Tints += Copied.Vertices[I].Col.A != Fresh.Vertices[I].Col.A || Copied.Vertices[I].Col.R != Fresh.Vertices[I].Col.R ? 1 : 0;
	}
	Check(Same, "capture: a copied-in Studio frame has the fresh capture's vertices, indices, commands and text, run for run");
	std::printf("  capture: copied vs fresh, %zu vertices; the client's own animation in 1/30 s moved %d (in %.0f,%.0f - %.0f,%.0f, at most %.2f) and tinted %d\n", Copied.Vertices.size(), Moved, Mx0, My0,
		Mx1, My1, Dmax, Tints);
	// Only the client is captured: the overlay over it draws live on a copied frame (here Hold or Crack's bar, opened 1/30 s after
	// the capture, with its counts).
	{
		D.Now += 1.0;
		D.Frame(); // captured
		kast::View V = D.S.Stream.GetView();
		V.HoC.Open = true;
		V.HoC.Holds = 2;
		V.HoC.Cracks = 1;
		V.HoC.Equity = 0.6;
		D.S.Stream.DebugView(V);
		long long Ca = 0, Pa = 0, Cb = 0, Pb = 0;
		kw::KastCaptureCounts(&D.RL, Ca, Pa);
		D.Now += 1.0 / 30.0;
		const DrawList Lv = D.Frame();
		kw::KastCaptureCounts(&D.RL, Cb, Pb);
		Check(Pb == Pa + 1 && Cb == Ca && RunIn(Lv, copy::HoldOrCrackTitle(), D.M, StudioPrev).Found && RunIn(Lv, copy::HoldOrCrackCounts(2, 1), D.M, StudioPrev).Found,
			"capture: a copied frame draws the overlay live (Hold or Crack's bar opened since the capture)");
		V.HoC = kast::View::HoldCrack();
		D.S.Stream.DebugView(V);
	}
	// The key as RiverLine::StreamPreview fills it: the laptop drawn under another transform, then at another pixel scale, re-captures
	// at once (a copy would put the old picture where the old transform drew it).
	{
		auto FrameAt = [&](float Dx, float Px) {
			DrawList Lx;
			ss::ui::Canvas Cx(Lx, D.M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Px);
			Cx.Translate(Dx, 0.0f);
			D.RL.Draw(Cx, D.Now);
			D.RL.UI.Ptr.EndFrame();
			return Lx;
		};
		D.Now += 1.0;
		D.Frame(); // captured
		long long Ca = 0, Pa = 0, Cb = 0, Pb = 0, Cc = 0, Pc = 0;
		kw::KastCaptureCounts(&D.RL, Ca, Pa);
		D.Now += 1.0 / 30.0;
		FrameAt(40.0f, 1.0f);
		kw::KastCaptureCounts(&D.RL, Cb, Pb);
		D.Now += 1.0 / 30.0;
		FrameAt(40.0f, 2.0f);
		kw::KastCaptureCounts(&D.RL, Cc, Pc);
		Check(Cb == Ca + 1 && Pb == Pa, "capture: the laptop drawn under another transform re-captures at once");
		Check(Cc == Cb + 1 && Pc == Pb, "capture: the laptop drawn at another pixel scale re-captures at once");
	}
}

void Budgets()
{
	// SPEC_CREATORS §9.3: each Kast page <= 1.5 ms warm, <= 2.0 ms at a final table with the Broadcast and Hold or Crack open.
	// The budget reads the thread's own CPU time (the wall clock moves with whatever else the machine runs; both are printed),
	// and is asserted under SS_UI_BENCH=1, the quiet-machine run the gate's bench is (riverline_ui's rule): with six builds on the
	// machine the same frame of today's Studio reads 1.3-2.1 ms. Warm is the game's: the laptop redraws 30 times a second and the
	// stream preview re-captures its client 15 (every other draw); "capture" is a draw that re-captures, printed beside it (the
	// Studio's preview of RiverLine's lobby is most of that frame: about 2/3 of it, the Kast widgets the rest). C-K's M5: the
	// mean is asserted; "max" (each batch's worst draw, the best batch's: on the Studio a re-capturing draw) and "max50" (the
	// worst of all 50 warm draws) are reported beside it, and studio_table benches the Studio at a table.
	static const bool Assert = std::getenv("SS_UI_BENCH") != nullptr;
	for (const KBench& B : Bench)
	{
		const bool Final = B.Name == "studio_event" || B.Name == "table_event";
		std::printf("  kast ui bench  %-18s %6.3f ms/draw cpu  %6.3f wall  %6.3f capture  %6.3f max  %6.3f max50  %6zu vertices  %2lld of 50 copied (best of 5 x 10)\n", B.Name.c_str(), B.Cpu,
			B.Wall, B.Capture, B.Max, B.MaxAll, B.Vertices, B.Copies);
		if (Assert && B.Cpu > (Final ? 2.0 : 1.5))
		{
			std::printf("FAILED: %s draws in %.3f ms (budget %.1f)\n", B.Name.c_str(), B.Cpu, Final ? 2.0 : 1.5);
			++Failures;
		}
		++Checks;
	}
}
} // namespace kast_seg

void SegKast()
{
	kast_seg::OneCount();
	kast_seg::CountUp();
	kast_seg::HonestZero();
	kast_seg::FirstStream();
	kast_seg::Heat();
	kast_seg::EventMode();
	kast_seg::TableSide();
	kast_seg::NameMenu();
	kast_seg::EndCard();
	kast_seg::Browse();
	kast_seg::Community();
	kast_seg::ChannelRules();
	kast_seg::EdgeStudio();
	kast_seg::EdgeCard();
	kast_seg::EdgeMoney();
	kast_seg::EdgeNod();
	kast_seg::EdgeCapture();
	kast_seg::FixGhost();
	kast_seg::FixHeadlines();
	kast_seg::FixL11();
	kast_seg::FixCopy();
	kast_seg::Copy();
	kast_seg::Budgets();
	std::printf("  kast-ui: %d checks\n", kast_seg::Checks);
}
} // namespace ui_test
