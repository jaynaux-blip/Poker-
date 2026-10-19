// riverline_ui: ui-video's additions (update 2 SPEC §4.8, §6.7, §8.2, §8.6): video_studio, video_studio_storytime,
// video_channel_gate, video_channel_partner (with the 28-day chart), video_empty, every frame by every kind group at
// 640 x 360, kast_summary_cut. Compiled into ui_test and called from main() in ui_main.cpp (the kit is UiSeg.h); the
// helpers live in ui_test::video_seg.
//
// What's checked, through the laptop's draw lists and the session (Rerun's side state is RiverLineVideo.cpp's; the test
// reaches it only through the entry points the laptop's other files call):
// - the empty states (§4.8); the taskbar's dot (three new candidates, or a milestone not yet seen) and the bell;
// - Studio: the rows best first with their kind, chips (AUTO, LIVE, the site, STORYTIME) and days left; the real title
//   (video::Title) and the real frame at 640 x 360 (videoart::Thumbnail); "Edit: 1 h 30 m · 8 energy"; PUBLISH through
//   Session::Publish (an hour and a half, 8 energy, the video under the title Studio showed); PUBLISH greyed with the
//   session's own words (tired, mid-stream, tables open) and the choice's (a Vlog short of three); a Vlog from one night
//   and a Recap of one run, multi-selected; the advice column never promises a number; storytime names nobody;
// - Channel: the partner gate's two bars, the 28-day chart (a bar for every day with views, subscribers or revenue, and
//   none for a day without), earnings once in, Partner's mark, the balance and the payouts, deleting with a second press;
// - every thumbnail frame for every kind group, inside its 640 x 360 box, its big words the hand's own;
// - Kast's stream summary: Cut highlights (N) opens Studio filtered to tonight's stream;
// - everything fits the laptop's 1600-unit width and lays out the same at 1920 and 2552 pixels wide; the copy lint;
// - (review-ui-video) an old save looked over writes no video line; the review week (no mark, no money, its line whole);
//   a pre-gate video earns only from the gate on; PUBLISH drawn greyed; a ninth Vlog moment refused; a click off the bell
//   does nothing under it; the dot after a load (the last 12 hours new); a series event's long name keeps the pot.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
// RiverLineVideo.cpp's entry points (the laptop's own files declare them the same way).
void VideoOpen(RiverLine& RL, int Page, double Now);
void VideoOpenTonight(RiverLine& RL, double From, double To, double Now);
bool VideoDot(const RiverLine* RL, const Session& S);
std::vector<std::string> VideoUiInventory();
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss

namespace ui_test
{
namespace video_seg
{
using ss::ui::DrawList;
namespace video = ss::video;

/** Where Studio draws the thumbnail (RiverLineVideo.cpp: the editor's column, centred). */
const ss::ui::Rect ThumbBox{490.0f, 400.0f, 640.0f, 360.0f};
/** The Channel's chart card and its plots (views, subscribers, revenue). */
const float ChartPx = 830.0f;
const float ChartPw = 730.0f;
const float PlotTop[3] = {278.0f, 370.0f, 426.0f};
const float PlotH[3] = {78.0f, 42.0f, 42.0f};

int Checks = 0;
void Check(bool Condition, const char* What)
{
	++Checks;
	Expect(Condition, What);
}

/** A laptop on the desk with Rerun's session, the pointer driven by hand, a frame drawn for every step. */
struct Desk
{
	QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit Desk(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
		Rest();
	}
	void Rest()
	{
		S.Life.Energy = 90.0;
		S.Life.Hunger = 5.0;
		S.Life.Thirst = 5.0;
	}
	DrawList Frame(float X = -1.0f, float Y = -1.0f, bool Down = false, bool Pressed = false, bool Released = false, float Wheel = 0.0f, float Scale = 1.0f)
	{
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		RL.UI.Ptr.Wheel = Wheel;
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Scale);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	void Click(float X, float Y)
	{
		Frame(X, Y);
		Now += 0.05;
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	}
	/** Runs the session until Done() (a frame each step) or the guard; true when it happened. */
	bool Until(const std::function<bool()>& Done, double MaxSeconds)
	{
		const double End = Now + MaxSeconds;
		while (!Done() && Now < End)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			Frame();
		}
		return Done();
	}
	void Open(int Page) { ss::ui::rlvideo_detail::VideoOpen(RL, Page, Now); }
	bool Dot() const { return ss::ui::rlvideo_detail::VideoDot(&RL, S); }
	std::string Fake(const std::string& What) { return S.DebugVideo("fake " + What); }
	/** Publishes through the session and lets the edit's time skip run out (the result card put away). */
	bool Publish(const video::Choice& C)
	{
		if (!S.Publish(C).empty())
		{
			return false;
		}
		const bool Done = Until([this]() { return !S.TimeSkip.Active; }, 15.0);
		S.HasOutcome = false;
		return Done;
	}
};

/** A text run that is exactly Text (or holds it, Has): where it sits and how wide it is (the measurer's). */
struct VRun
{
	bool Found = false;
	float X = 0.0f, Base = 0.0f, W = 0.0f, Size = 0.0f;
	size_t Index = 0;
};
VRun RunOf(const DrawList& L, const std::string& Text, const TableMeasurer& M, float MinY = -1.0e9f, float MaxY = 1.0e9f, bool Has = false)
{
	VRun Out;
	for (size_t I = 0; I < L.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[I];
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < MinY || Cmd.Text.BaselineY > MaxY)
		{
			continue;
		}
		if (Has ? Cmd.Text.Text.find(Text) != std::string::npos : Cmd.Text.Text == Text)
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.Size = Cmd.Text.SizePx;
			Out.W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
			Out.Index = I;
			return Out;
		}
	}
	return Out;
}

/** Clicks the middle of a run (a tab, a segment, a row's kind, a button's label): false when it isn't drawn. */
bool ClickRun(Desk& D, const DrawList& L, const std::string& Text, float MinY = -1.0e9f, float MaxY = 1.0e9f)
{
	const VRun R = RunOf(L, Text, D.M, MinY, MaxY);
	if (R.Found)
	{
		D.Click(R.X + R.W / 2.0f, R.Base - R.Size * 0.35f);
	}
	return R.Found;
}

/** The baseline where NetSpaced's tracked capitals (one run a glyph) spell Word, or -1. */
float SpelledAt(const DrawList& L, const std::string& Word)
{
	std::string Cur;
	float Base = -1.0e9f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		if (T.size() == 1 && Cmd.Text.BaselineY == Base)
		{
			Cur += T;
		}
		else
		{
			Cur = T.size() == 1 ? T : std::string();
			Base = Cmd.Text.BaselineY;
		}
		if (Cur.find(Word) != std::string::npos)
		{
			return Base;
		}
	}
	return -1.0f;
}

bool Spelled(const DrawList& L, const std::string& Word)
{
	return SpelledAt(L, Word) >= 0.0f;
}

/** Every text run, newline-separated (a quick "is it anywhere" for the privacy and brand checks). */
std::string Joined(const DrawList& L)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out += Cmd.Text.Text + "\n";
		}
	}
	return Out;
}

bool Inside(const ss::ui::Rect& R, float X, float Y)
{
	return X >= R.X && X <= R.X + R.W && Y >= R.Y && Y <= R.Y + R.H;
}

/** The command range of Studio's thumbnail: its clip at 640 x 360 and what's drawn inside it. First is -1 when absent. */
struct ClipSpan
{
	long First = -1, Last = -1;
	size_t Vertices = 0;
};
ClipSpan ThumbSpan(const DrawList& L, const ss::ui::Rect& Box = ThumbBox)
{
	ClipSpan Out;
	int Depth = 0;
	for (size_t I = 0; I < L.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[I];
		if (Out.First < 0)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::PushClip && std::fabs(Cmd.Clip.X - Box.X) < 0.5f && std::fabs(Cmd.Clip.Y - Box.Y) < 0.5f && std::fabs(Cmd.Clip.W - Box.W) < 0.5f &&
				std::fabs(Cmd.Clip.H - Box.H) < 0.5f)
			{
				Out.First = static_cast<long>(I);
				Depth = 1;
			}
			continue;
		}
		Depth += Cmd.Type == ss::ui::DrawCmd::Kind::PushClip ? 1 : Cmd.Type == ss::ui::DrawCmd::Kind::PopClip ? -1 : 0;
		Out.Vertices += Cmd.Type == ss::ui::DrawCmd::Kind::Triangles ? Cmd.VertexCount : 0;
		if (Depth == 0)
		{
			Out.Last = static_cast<long>(I);
			break;
		}
	}
	return Out;
}

/** A run that is exactly Text inside the thumbnail's clip. */
bool InThumb(const DrawList& L, const std::string& Text)
{
	const ClipSpan Sp = ThumbSpan(L);
	for (long I = Sp.First; Sp.First >= 0 && I <= Sp.Last; ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[static_cast<size_t>(I)];
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Inside(ThumbBox, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY))
		{
			return true;
		}
	}
	return false;
}

/** The runs inside the thumbnail's clip, joined. */
std::string ThumbText(const DrawList& L)
{
	std::string Out;
	const ClipSpan Sp = ThumbSpan(L);
	for (long I = Sp.First; Sp.First >= 0 && I <= Sp.Last; ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[static_cast<size_t>(I)];
		Out += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? Cmd.Text.Text + "\n" : std::string();
	}
	return Out;
}

/** The runs whose start lies in Region, joined (the advice column, a card). */
std::string TextIn(const DrawList& L, const ss::ui::Rect& Region)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Inside(Region, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY))
		{
			Out += Cmd.Text.Text + "\n";
		}
	}
	return Out;
}

/**
 * Text runs starting inside Region must end inside it, left and right (skipping the runs inside Skip, a thumbnail whose
 * words the frame shrinks and clips itself): the count of those that don't, each printed.
 */
int Overflows(const DrawList& L, const ss::ui::Rect& Region, const TableMeasurer& M, const char* What, const ss::ui::Rect& Skip = {0.0f, 0.0f, 0.0f, 0.0f})
{
	int Bad = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || !Inside(Region, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY) || (Skip.W > 0.0f && Inside(Skip, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY)))
		{
			continue;
		}
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < Region.X - 0.5f || Cmd.Text.X + W > Region.X + Region.W + 0.5f)
		{
			++Bad;
			std::printf("  %s: \"%s\" runs %.1f..%.1f outside %.1f..%.1f\n", What, Cmd.Text.Text.c_str(), Cmd.Text.X, Cmd.Text.X + W, Region.X, Region.X + Region.W);
		}
	}
	return Bad;
}

bool SameColour(const ss::ui::Color& A, const ss::ui::Color& B)
{
	return std::fabs(A.R - B.R) < 0.01f && std::fabs(A.G - B.G) < 0.01f && std::fabs(A.B - B.B) < 0.01f;
}

/** The chart's days with a bar in plot Row (by the bar's colour inside the plot): one flag per day, oldest first. */
std::vector<bool> BarsIn(const DrawList& L, int Row, const ss::ui::Color& Col)
{
	std::vector<bool> Out(28, false);
	const float Slot = ChartPw / 28.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles)
		{
			continue;
		}
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			if (V.Col.A < 0.4f || !SameColour(V.Col, Col) || V.X < ChartPx || V.X >= ChartPx + ChartPw || V.Y < PlotTop[Row] - 0.5f || V.Y > PlotTop[Row] + PlotH[Row] + 0.5f)
			{
				continue;
			}
			Out[static_cast<size_t>(std::clamp(static_cast<int>((V.X - ChartPx) / Slot), 0, 27))] = true;
		}
	}
	return Out;
}

std::string Floored(double V)
{
	return ss::Grouped(static_cast<int64_t>(std::floor(V)));
}

video::Choice ChoiceOf(const std::vector<std::string>& Ids, video::Format F, video::TitleStyle T = video::TitleStyle::Story, video::Frame Fr = video::Frame::Reaction)
{
	video::Choice C;
	C.Candidates = Ids;
	C.F = F;
	C.T = T;
	C.Fr = Fr;
	return C;
}

const video::Candidate* FindKind(const ss::Session& S, video::Kind K)
{
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		if (C.K == K)
		{
			return &C;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ video_empty

void Empty()
{
	Desk D("rerun-empty");
	Check(!D.Dot(), "video_empty: no taskbar dot on a channel with nothing new");
	D.Open(0);
	DrawList L = D.Frame();
	Check(DrewRun(L, "Rerun") && DrewRun(L, "Studio") && DrewRun(L, "Channel"), "video_empty: Rerun's header with its two pages");
	Check(DrewRun(L, "Play some hands. The good ones show up here."), "video_empty: Studio's empty state (§4.8)");
	Check(!Drew(L, "PUBLISH") && !Spelled(L, "FORMAT"), "video_empty: no editor with nothing to cut");
	WriteList("video_empty", L);
	Check(ClickRun(D, L, "Channel", 0.0f, 60.0f), "video_empty: the Channel tab");
	L = D.Frame();
	Check(DrewRun(L, "No videos yet."), "video_empty: the Channel's empty state (§4.8, §8.2's old save)");
	Check(DrewRun(L, "Subscribers") && DrewRun(L, "0 / 1,000") && DrewRun(L, "Watch hours") && DrewRun(L, "0 / 4,000"), "video_empty: the gate's two bars at zero");
	Check(DrewRun(L, "Last 28 days") && DrewRun(L, "Not yet") && DrewRun(L, "Ads start at the gate."), "video_empty: the chart and the program, honest at zero");
	Check(!DrewRun(L, "PARTNER") && !Drew(L, "earned") && !Drew(L, "Open Studio"), "video_empty: no Partner mark, no earnings, no Studio button with nothing to cut");
	WriteList("video_empty_channel", L);
	std::printf("  video_empty         %5zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
}

// ------------------------------------------------------------------ video_studio

void StudioFlow()
{
	Desk D("rerun-studio");
	D.Fake("badbeat online");
	D.Fake("bluff embercrest");
	Check(!D.Dot(), "video_studio: two new candidates don't light the dot yet");
	D.Fake("bigpot skip");
	Check(D.Dot(), "video_studio: the third new candidate lights the taskbar's dot (§4.9)");
	D.Open(0);
	DrawList L = D.Frame();
	Check(!D.Dot(), "video_studio: a visit to Rerun puts the dot out");
	const std::vector<video::Candidate>& Open = D.S.Life.Video.Open;
	const std::vector<video::Candidate> List = D.S.VideoCandidates();
	// The rows, best first: each one's kind, at rising baselines in the session's Drama order.
	float Last = -1.0f;
	bool Ordered = List.size() == 3;
	for (const video::Candidate& C : List)
	{
		const VRun R = RunOf(L, video::KindLabel(C.K), D.M, 100.0f, 960.0f);
		Ordered = Ordered && R.Found && R.X > 20.0f && R.X < 140.0f && R.Base > Last;
		Last = R.Base;
	}
	Check(Ordered, "video_studio: the candidates listed by Drama, best first, each with its kind");
	Check(DrewRun(L, "AUTO") && DrewRun(L, "LIVE") && DrewRun(L, "RIVERLINE"), "video_studio: the AUTO chip (the skipped hand), LIVE (the Embercrest), the site (RiverLine)");
	const VRun Auto = RunOf(L, "AUTO", D.M), Big = RunOf(L, "BIG POT", D.M, 100.0f, 960.0f);
	Check(Auto.Found && Big.Found && std::fabs(Auto.Base - Big.Base) < 14.0f, "video_studio: AUTO sits on the skipped hand's row only");
	Check(DrewRun(L, "21 days left") || DrewRun(L, "20 days left"), "video_studio: each row's days left (kept 21 days)");
	Check(Drew(L, "Night Owl Turbo \xC2\xB7 ") && Drew(L, " BB"), "video_studio: what happened, with the pot in big blinds");
	// The editor: the best candidate picked, a Breakdown with the Story title and the Reaction frame (the defaults).
	const video::Choice Ch = ChoiceOf({List.front().Id}, video::Format::Breakdown);
	const std::string Title = video::Title(Ch, Open, 0);
	const std::string Quoted = "\xE2\x80\x9C" + Title + "\xE2\x80\x9D";
	Check(!Title.empty() && (DrewRun(L, Quoted) || Drew(L, Quoted.substr(0, 14))), "video_studio: the real title, made from the hand (§4.5: no free text)");
	Check(DrewRun(L, "Edit: 1 h 30 m \xC2\xB7 8 energy"), "video_studio: \"Edit: 1 h 30 m · 8 energy\" (§4.8)");
	const ClipSpan Sp = ThumbSpan(L);
	Check(Sp.First >= 0 && Sp.Vertices > 200, "video_studio: the thumbnail drawn at 640 x 360 (videoart::Thumbnail)");
	const video::Thumb Th = video::ThumbFor(Ch, Open);
	Check(!Th.BigText.empty() && InThumb(L, Th.BigText), "video_studio: the frame's big words are the hand's own");
	// The right-hand column: a line on each choice, never a promised number (§4.8).
	const std::vector<std::string> Advice = video::Advice(Ch, Open);
	bool AllAdvice = !Advice.empty();
	for (const std::string& A : Advice)
	{
		AllAdvice = AllAdvice && Drew(L, A.substr(0, std::min<size_t>(A.size(), 22)));
	}
	Check(AllAdvice, "video_studio: video::Advice's lines in the right-hand column");
	const float TileLabel = SpelledAt(L, "ON YOUR CHANNEL");
	const std::string Side = TextIn(L, {1180.0f, 110.0f, 400.0f, (TileLabel > 0.0f ? TileLabel : 470.0f) - 120.0f});
	Check(!Side.empty() && Side.find_first_of("0123456789%") == std::string::npos, "video_studio: the advice never promises a number");
	Check(Spelled(L, "ON YOUR CHANNEL") && Drew(L, "0 views \xC2\xB7 just now"), "video_studio: the tile as the Channel will show it");
	Check(!Drew(L, "Close your tables") && !Drew(L, "Too tired") && !Drew(L, "End the stream"), "video_studio: no reason under PUBLISH when it can go");
	WriteList("video_studio", L);
	std::printf("  video_studio        %5zu vertices %5zu commands  \"%s\"\n", L.Vertices.size(), L.Cmds.size(), Title.c_str());
	// The skipped hand: AUTO for the player in Studio, never in the video's own words (U1).
	{
		const video::Candidate* Skip = FindKind(D.S, video::Kind::BigPot);
		Check(Skip && Skip->Auto && ClickRun(D, L, "BIG POT", 100.0f, 960.0f), "video_studio: the skipped hand can be picked");
		const DrawList La = D.Frame();
		const std::string Words = video::Title(ChoiceOf({Skip ? Skip->Id : std::string()}, video::Format::Breakdown), Open, 0) + "\n" + ThumbText(La);
		std::string Low = Words;
		std::transform(Low.begin(), Low.end(), Low.begin(), [](char C) { return static_cast<char>(C >= 'A' && C <= 'Z' ? C - 'A' + 'a' : C); });
		Check(Low.find("auto") == std::string::npos && Low.find(" bot") == std::string::npos && Low.find(" ai ") == std::string::npos, "video_studio: the title and the frame never say the plan played");
		Check(Drew(La, "Your plan played this one. The video never says so."), "video_studio: Studio tells the player, and only the player");
		// Back to the best one for the publish below.
		const VRun Best = RunOf(La, video::KindLabel(List.front().K), D.M, 100.0f, 960.0f);
		D.Click(Best.X + 4.0f, Best.Base - 5.0f);
		L = D.Frame();
	}
	// PUBLISH (§8.2 step 3): a Breakdown, Story, Reaction: the clock moves 1 h 30 m, 8 energy.
	const double W0 = D.S.WorldMinutes();
	const double E0 = D.S.Life.Energy;
	Check(ClickRun(D, L, "PUBLISH"), "video_studio: PUBLISH drawn");
	Check(D.S.TimeSkip.Active && std::fabs(D.S.TimeSkip.To - D.S.TimeSkip.From - 90.0) < 0.01, "video_studio: PUBLISH starts the edit, an hour and a half of the clock (a TimeSkip)");
	D.Until([&D]() { return !D.S.TimeSkip.Active; }, 15.0);
	Check(D.S.Life.Video.Videos.size() == 1 && D.S.Life.Video.Videos[0].Title == Title, "video_studio: the video goes up under the title Studio showed");
	Check(std::fabs(D.S.WorldMinutes() - W0 - 90.0) < 2.0 && std::fabs(D.S.Life.Energy - (E0 - 8.0)) < 1.0, "video_studio: the edit took 1 h 30 m and 8 energy");
	Check(D.S.Life.Video.Open.size() == 2, "video_studio: its moment is used up");
	D.S.HasOutcome = false;
	L = D.Frame();
	Check(Spelled(L, "YOUR VIDEOS") && Drew(L, Title.substr(0, std::min<size_t>(Title.size(), 10))), "video_studio: after publishing, the Channel shows the new video at once");
}

// ------------------------------------------------------------------ PUBLISH greyed, with the reason

void PublishReasons()
{
	Desk D("rerun-why");
	D.Fake("herocall online");
	D.Open(0);
	D.Frame();
	// Too tired (below 10 energy): Session::Publish's own words, and the press does nothing.
	D.S.Life.Energy = 5.0;
	DrawList L = D.Frame();
	Check(DrewRun(L, "Too tired to edit. Get some sleep first."), "PUBLISH greyed below 10 energy, with the session's reason");
	ClickRun(D, L, "PUBLISH");
	Check(!D.S.TimeSkip.Active && D.S.Life.Video.Videos.empty(), "PUBLISH greyed: a press does nothing");
	D.Rest();
	// Mid-stream.
	D.S.Stream.Live = true;
	L = D.Frame();
	Check(DrewRun(L, "End the stream first."), "PUBLISH greyed mid-stream");
	ClickRun(D, L, "PUBLISH");
	Check(!D.S.TimeSkip.Active, "PUBLISH greyed mid-stream: a press does nothing");
	D.S.Stream.Live = false;
	// The choice's own problem comes first: a Vlog of one moment.
	L = D.Frame();
	Check(ClickRun(D, L, "Vlog", 112.0f, 168.0f), "the Vlog format");
	L = D.Frame();
	Check(DrewRun(L, "A Vlog takes three to eight moments from one night."), "PUBLISH greyed: a Vlog needs three to eight moments from one night");
	Check(ClickRun(D, L, "Breakdown", 112.0f, 168.0f), "back to a Breakdown");
	L = D.Frame();
	Check(!Drew(L, "A Vlog takes"), "a Breakdown of the one hand is fine again");
	// Tables open.
	D.S.BankrollCents = 10000;
	D.S.Register(1);
	L = D.Frame();
	Check(D.S.TableCount() > 0 && DrewRun(L, "Close your tables first. Editing needs your whole attention."), "PUBLISH greyed with tables open");
	ClickRun(D, L, "PUBLISH");
	Check(!D.S.TimeSkip.Active, "PUBLISH greyed with tables open: a press does nothing");
}

// ------------------------------------------------------------------ a night's Vlog, a run's Recap

void Formats()
{
	Desk D("rerun-formats");
	D.Fake("badbeat online");
	D.Fake("bluff online");
	D.Fake("bigpot online");
	D.Fake("win online");
	D.S.LobbyMinutes = D.S.WorldMinutes() + 1440.0;
	D.Fake("cooler online");
	D.Open(0);
	DrawList L = D.Frame();
	const std::vector<video::Candidate>& Open = D.S.Life.Video.Open;
	const video::Candidate* Beat = FindKind(D.S, video::Kind::BadBeat);
	const video::Candidate* Bluff = FindKind(D.S, video::Kind::Bluff);
	const video::Candidate* Pot = FindKind(D.S, video::Kind::BigPot);
	const video::Candidate* Win = FindKind(D.S, video::Kind::Win);
	Check(Beat && Bluff && Pot && Win && FindKind(D.S, video::Kind::Cooler), "formats: five candidates, one from the next night");
	if (!Beat || !Bluff || !Pot || !Win)
	{
		return;
	}
	const std::string BeatId = Beat->Id, BluffId = Bluff->Id, PotId = Pot->Id, WinId = Win->Id;
	auto Row = [&](const DrawList& F, const char* Kind) { return ClickRun(D, F, Kind, 100.0f, 960.0f); };
	Check(Row(L, "BAD BEAT"), "formats: pick the bad beat");
	L = D.Frame();
	Check(ClickRun(D, L, "Vlog", 112.0f, 168.0f), "formats: the Vlog format");
	L = D.Frame();
	Check(DrewRun(L, "another night"), "formats: the next night's hand can't join tonight's Vlog, and says so");
	Check(Row(L, "BLUFF"), "formats: add the bluff");
	L = D.Frame();
	Check(Row(L, "BIG POT"), "formats: add the big pot");
	L = D.Frame();
	const video::Choice Vlog = ChoiceOf({BeatId, BluffId, PotId}, video::Format::Vlog);
	Check(video::CanUpload(Vlog, Open).empty() && !Drew(L, "A Vlog takes"), "formats: three moments from one night make a Vlog");
	const std::string VlogTitle = video::Title(Vlog, Open, 0);
	Check(Drew(L, ("\xE2\x80\x9C" + VlogTitle).substr(0, 14)), "formats: the Vlog's real title");
	Check(DrewRun(L, "Edit: 3 h \xC2\xB7 15 energy"), "formats: a Vlog's edit, 3 h and 15 energy");
	Check(DrewRun(L, "1") && DrewRun(L, "2") && DrewRun(L, "3"), "formats: the picks numbered in order");
	WriteList("video_studio_vlog", L);
	// A hand from the next night starts over from itself.
	Check(Row(L, "COOLER"), "formats: the cooler from the next night");
	L = D.Frame();
	Check(DrewRun(L, "A Vlog takes three to eight moments from one night."), "formats: a Vlog starts over from the other night's hand");
	// A Recap: the run's result joins the hand picked.
	Check(Row(L, "BAD BEAT"), "formats: back to the bad beat");
	L = D.Frame();
	Check(ClickRun(D, L, "Recap", 112.0f, 168.0f), "formats: the Recap format");
	L = D.Frame();
	Check(DrewRun(L, "A Recap needs a deep run, a final table or a win, plus two to six hands from that event."), "formats: a Recap needs two hands to go with its result");
	Check(Row(L, "BLUFF"), "formats: add a second hand");
	L = D.Frame();
	const video::Choice Recap = ChoiceOf({WinId, BeatId, BluffId}, video::Format::Recap);
	Check(video::CanUpload(Recap, Open).empty() && !Drew(L, "A Recap needs"), "formats: the win and two of its hands make a Recap (the result brought in with the format)");
	Check(Drew(L, ("\xE2\x80\x9C" + video::Title(Recap, Open, 0)).substr(0, 14)), "formats: the Recap's real title");
	Check(DrewRun(L, "Edit: 2 h 30 m \xC2\xB7 12 energy"), "formats: a Recap's edit, 2 h 30 m and 12 energy");
	WriteList("video_studio_recap", L);
	Check(ClickRun(D, L, "Short", 112.0f, 168.0f), "formats: a Short");
	L = D.Frame();
	Check(DrewRun(L, "Edit: 20 m \xC2\xB7 2 energy") && !Drew(L, "A Short is one moment"), "formats: a Short keeps one moment: 20 m and 2 energy");
	// The picked moment posted from elsewhere (ss.Video.Upload): Studio moves on to the best one left.
	Check(D.Publish(ChoiceOf({WinId}, video::Format::Short)), "formats: the win posted from the console");
	L = D.Frame();
	Check(!Drew(L, "Pick a moment on the left.") && Drew(L, "\xE2\x80\x9C"), "formats: a pick that's gone gives way to the best one left");
}

// ------------------------------------------------------------------ video_studio_storytime

void Storytime()
{
	Desk D("rerun-storytime");
	D.Fake("cooler dees");
	D.Fake("bluff dees");
	D.Open(0);
	DrawList L = D.Frame();
	Check(ClickRun(D, L, "COOLER", 100.0f, 960.0f), "storytime: pick Dee's cooler");
	L = D.Frame();
	Check(DrewRun(L, "STORYTIME"), "storytime: the STORYTIME chip on Dee's hands (§4.8)");
	Check(Drew(L, "A back-room game"), "storytime: the game is \"a back-room game\", never Dee's");
	Check(Drew(L, "Storytime: nobody from the game is named"), "storytime: the advice says how it's told");
	// Nobody from the game, in any frame: no cast name, no "Dee's" (the fake's villain is Big Lou).
	const char* Frames[5] = {"Reaction", "Cards", "Number", "Versus", "Trophy"};
	bool Nameless = true, Drawn = true;
	for (const char* Fr : Frames)
	{
		ClickRun(D, L, Fr, 354.0f, 388.0f);
		L = D.Frame();
		const std::string All = Joined(L);
		Nameless = Nameless && All.find("Big Lou") == std::string::npos && All.find("Dee's") == std::string::npos;
		Drawn = Drawn && ThumbSpan(L).Vertices > 100;
		if (std::string(Fr) == "Versus")
		{
			Check(InThumb(L, "?"), "storytime: the Versus frame's opponent is a silhouette with a question mark");
		}
		if (std::string(Fr) == "Reaction")
		{
			WriteList("video_studio_storytime", L);
		}
	}
	Check(Nameless, "storytime: no frame, title or line names anyone from the game");
	Check(Drawn, "storytime: every frame draws");
}

// ------------------------------------------------------------------ video_channel_gate

void ChannelGate()
{
	Desk D("rerun-gate");
	D.S.DebugVideo("grant 612 1840 0");
	D.Fake("badbeat online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Breakdown)), "gate: a Breakdown goes up");
	D.Fake("bigpot online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Short, video::TitleStyle::Hype, video::Frame::Number)), "gate: a Short goes up");
	D.S.DebugVideo("advance 14");
	D.Open(1);
	DrawList L = D.Frame();
	const video::ChannelView V = D.S.VideoView();
	const std::string Subs = Floored(V.Subs) + " / 1,000";
	const std::string Hours = Floored(V.WatchHours365) + " / 4,000";
	std::printf("  video_channel_gate  Subscribers %s, Watch hours %s, %s views in 28 days\n", Subs.c_str(), Hours.c_str(), Floored(V.Views28).c_str());
	Check(!V.Partner && V.Subs >= 612.0 && V.Subs < 1000.0 && V.WatchHours365 >= 1840.0 && V.WatchHours365 < 4000.0, "gate: under the gate on both bars");
	Check(DrewRun(L, "Subscribers") && DrewRun(L, Subs) && DrewRun(L, "Watch hours") && DrewRun(L, Hours), "gate: \"Subscribers N / 1,000 · Watch hours N / 4,000\" (§4.1)");
	Check(DrewRun(L, "Fill both bars and Rerun takes a look.") && DrewRun(L, "Not yet"), "gate: what the bars are for, and no promise past them");
	// The 28-day chart: the totals, and a bar for every day with views (none for a day without).
	double S28 = 0.0;
	for (const double X : V.SubsByDay)
	{
		S28 += X;
	}
	Check(Spelled(L, "VIEWS") && DrewRun(L, Floored(V.Views28)) && Spelled(L, "SUBSCRIBERS") && DrewRun(L, "+" + Floored(S28)), "gate: the chart's 28-day totals");
	const std::vector<bool> Bars = BarsIn(L, 0, ss::ui::Hex(ss::ui::videoart::Marigold));
	int Match = 0, Days = 0;
	for (size_t I = 0; I < 28; ++I)
	{
		Days += V.ViewsByDay[I] > 0.0 ? 1 : 0;
		Match += (V.ViewsByDay[I] > 0.0) == Bars[I] || I == 27 ? 1 : 0; // today's bar is drawn fainter
	}
	Check(Days >= 14 && Match == 28, "gate: a views bar for each day that had views, none for a day without");
	Check(DrewRun(L, "Ads start at the gate.") && !Drew(L, "earned") && !DrewRun(L, "PARTNER"), "gate: no revenue, no earnings and no Partner mark before the gate");
	Check(Drew(L, " views \xC2\xB7 ") && Drew(L, "days ago"), "gate: each video's views and age");
	// A day of the chart under the pointer: its date, views and subscribers.
	int Hot = 0;
	for (int I = 0; I < 27; ++I)
	{
		Hot = V.ViewsByDay[static_cast<size_t>(I)] > 0.0 ? I : Hot;
	}
	const DrawList Lh = D.Frame(ChartPx + (static_cast<float>(Hot) + 0.5f) * ChartPw / 28.0f, 320.0f);
	const int Today = ss::net::DayOf(D.S.WorldMinutes());
	Check(Drew(Lh, ss::net::DateLabel(Today - 27 + Hot) + " \xC2\xB7 ") && Drew(Lh, " subscribers"), "gate: the day under the pointer, with its views and subscribers");
	WriteList("video_channel_gate", L);
	std::printf("  video_channel_gate  %5zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
	// Taking a video down: the first press asks, the second does it (its subscribers stay, §4.5).
	const double SubsBefore = D.S.Life.Video.Subs;
	const size_t Count = D.S.Life.Video.Videos.size();
	const float TileW = (1560.0f - 6.0f * 14.0f) / 7.0f;
	D.Frame(20.0f + TileW / 2.0f, 600.0f);
	L = D.Frame(20.0f + TileW / 2.0f, 600.0f);
	Check(ClickRun(D, L, "Delete"), "delete: a tile under the pointer offers Delete");
	L = D.Frame(20.0f + TileW / 2.0f, 600.0f);
	Check(DrewRun(L, "Press again to take it down.") && DrewRun(L, "Sure?") && D.S.Life.Video.Videos.size() == Count, "delete: the first press only asks");
	Check(ClickRun(D, L, "Sure?"), "delete: the second press");
	L = D.Frame();
	Check(D.S.Life.Video.Videos.size() == Count - 1 && D.S.Life.Video.Subs == SubsBefore, "delete: the video's gone, its subscribers stay");
	Check(DrewRun(L, "Taken down. The subscribers it brought in stay."), "delete: and the Channel says so");
}

// ------------------------------------------------------------------ video_channel_partner

void ChannelPartner()
{
	Desk D("rerun-partner");
	D.Fake("badbeat online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Breakdown)), "partner: a Breakdown before the gate");
	D.S.DebugVideo("grant 1000 4000 60000");
	D.S.DebugVideo("advance 30");
	Check(D.S.Life.Video.PartnerAt >= 0.0, "partner: the review, then the Partner Program (§8.2 step 4)");
	D.Fake("bluff online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Breakdown, video::TitleStyle::Teaching, video::Frame::Cards)), "partner: a Breakdown after it");
	D.S.DebugVideo("advance 45");
	D.Open(1);
	DrawList L = D.Frame();
	const video::ChannelView V = D.S.VideoView();
	const video::Channel& Ch = D.S.Life.Video;
	Check(V.Partner && DrewRun(L, "PARTNER") && DrewRun(L, "You're a Rerun Partner."), "partner: the banner's Partner mark, once in (§4.8)");
	Check(Ch.PaidCents > 0 && DrewRun(L, "paid out " + ss::Money(Ch.PaidCents) + " so far"), "partner: what's been paid out (the 21st's payout)");
	Check(Spelled(L, "BALANCE") && DrewRun(L, ss::Money(V.Balance)), "partner: the balance waiting for the next 21st");
	double Cents = 0.0;
	for (const double X : V.CentsByDay)
	{
		Cents += X;
	}
	Check(Spelled(L, "REVENUE") && DrewRun(L, ss::Money(static_cast<ss::Chips>(std::floor(Cents)))), "partner: the chart's revenue row, its 28-day total");
	const std::vector<bool> Bars = BarsIn(L, 2, ss::ui::Hex(0xffd479));
	int Match = 0, Days = 0;
	for (size_t I = 0; I < 28; ++I)
	{
		Days += V.CentsByDay[I] > 0.0 ? 1 : 0;
		Match += (V.CentsByDay[I] > 0.0) == Bars[I] || I == 27 ? 1 : 0;
	}
	Check(Days >= 20 && Match == 28, "partner: a revenue bar for each day that earned");
	// Each video's earnings, once in: the one posted after the gate has earned, from its own RPM.
	const video::Video* After = nullptr;
	for (const video::Video& X : Ch.Videos)
	{
		After = X.At > Ch.PartnerAt && X.Title != "Granted views (dev)" ? &X : After;
	}
	Check(After != nullptr, "partner: the video posted after the gate");
	if (After)
	{
		const double Earned = std::max(0.0, video::Views(*After, D.S.WorldMinutes()) - video::Views(*After, After->At)) * After->Rpm / 1000.0 * 100.0;
		Check(Earned >= 1.0 && DrewRun(L, ss::Money(static_cast<ss::Chips>(std::floor(Earned))) + " earned"), "partner: each tile's earnings since the gate");
	}
	WriteList("video_channel_partner", L);
	std::printf("  video_channel_partner %5zu vertices %5zu commands  paid %s, balance %s\n", L.Vertices.size(), L.Cmds.size(), ss::Money(Ch.PaidCents).c_str(), ss::Money(V.Balance).c_str());
}

// ------------------------------------------------------------------ every frame by every kind group at 640 x 360

void Frames()
{
	Desk D("rerun-frames");
	// One kind from each of the frame table's groups (§4.5): beats, reads, pots, runs, the rival, stories.
	for (const char* K : {"badbeat online", "bluff online", "bigpot online", "win online", "rival online"})
	{
		D.Fake(K);
	}
	D.S.Life.Update2LinesRead |= ss::life::U2Video;
	video::NoteStory(D.S.Life.Video, "rent-paid", D.S.WorldMinutes());
	D.Open(0);
	DrawList L = D.Frame();
	const char* Groups[6] = {"BAD BEAT", "BLUFF", "BIG POT", "WIN", "RIVAL", "STORY"};
	const char* Frames[5] = {"Reaction", "Cards", "Number", "Versus", "Trophy"};
	int Drawn = 0, Words = 0, Inside = 0;
	for (const char* G : Groups)
	{
		ClickRun(D, L, G, 100.0f, 960.0f);
		for (const char* Fr : Frames)
		{
			L = D.Frame();
			ClickRun(D, L, Fr, 354.0f, 388.0f);
			L = D.Frame();
			const ClipSpan Sp = ThumbSpan(L);
			Drawn += Sp.First >= 0 && Sp.Vertices > 100 ? 1 : 0;
			// The frame's big words, from the same choice Studio built (the pick is this group's row, the frame this one).
			const video::Candidate* C = nullptr;
			for (const video::Candidate& X : D.S.Life.Video.Open)
			{
				C = std::string(video::KindLabel(X.K)) == G ? &X : C;
			}
			int Fi = 0;
			while (Fi < 5 && std::string(Frames[Fi]) != Fr)
			{
				++Fi;
			}
			const video::Thumb Th = C ? video::ThumbFor(ChoiceOf({C->Id}, video::Format::Breakdown, video::TitleStyle::Story, static_cast<video::Frame>(Fi)), D.S.Life.Video.Open) : video::Thumb();
			Words += C && (Th.BigText.empty() || InThumb(L, Th.BigText)) ? 1 : 0;
			// Everything the frame draws stays in its box.
			bool In = Sp.First >= 0;
			for (long I = Sp.First; In && I <= Sp.Last; ++I)
			{
				const ss::ui::DrawCmd& Cmd = L.Cmds[static_cast<size_t>(I)];
				In = Cmd.Type != ss::ui::DrawCmd::Kind::PushClip || (Cmd.Clip.X >= ThumbBox.X - 0.5f && Cmd.Clip.Y >= ThumbBox.Y - 0.5f);
			}
			Inside += In ? 1 : 0;
			if (std::string(G) == "BAD BEAT")
			{
				WriteList(std::string("video_frame_badbeat_") + Fr, L);
				// How far the frame's own drawing runs past its right edge (the clip hides it; information, not a check:
				// videoart's Reaction frame puts the opponent's shown cards past it, patches/ui-video_to_lead_1.diff).
				float Right = 0.0f;
				for (long I = Sp.First; Sp.First >= 0 && I <= Sp.Last; ++I)
				{
					const ss::ui::DrawCmd& Cmd = L.Cmds[static_cast<size_t>(I)];
					for (uint32_t K = 0; Cmd.Type == ss::ui::DrawCmd::Kind::Triangles && K < Cmd.VertexCount; ++K)
					{
						const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
						Right = V.Col.A > 0.5f ? std::max(Right, V.X) : Right;
					}
				}
				std::printf("  video frame %-8s draws to x %.0f (the box ends at %.0f)\n", Fr, Right, ThumbBox.X + ThumbBox.W);
			}
		}
	}
	std::printf("  video frames        %d of 30 drawn, %d with the hand's own big words, %d inside the box\n", Drawn, Words, Inside);
	Check(Drawn == 30, "frames: every frame for every kind group draws at 640 x 360");
	Check(Words == 30, "frames: each frame's big words are the moment's own (videoart's Thumb from video::ThumbFor)");
	Check(Inside == 30, "frames: every frame keeps inside its box");
	// VC2: every frame for one bad beat, side by side (5 x 640 x 360 on one sheet).
	const video::Candidate* Beat = FindKind(D.S, video::Kind::BadBeat);
	if (Beat)
	{
		DrawList Sheet;
		ss::ui::Canvas C(Sheet, D.M, 1320.0f, 1140.0f, 1.0f);
		C.FillRect({0.0f, 0.0f, 1320.0f, 1140.0f}, ss::ui::Hex(ss::ui::videoart::Ink));
		for (int Fi = 0; Fi < 5; ++Fi)
		{
			const ss::ui::Rect R{20.0f + static_cast<float>(Fi % 2) * 660.0f, 20.0f + static_cast<float>(Fi / 2) * 380.0f, 640.0f, 360.0f};
			C.PushClip(R);
			ss::ui::videoart::Thumbnail(C, R, video::ThumbFor(ChoiceOf({Beat->Id}, video::Format::Breakdown, video::TitleStyle::Story, static_cast<video::Frame>(Fi)), D.S.Life.Video.Open),
				&D.S.Person);
			C.PopClip();
		}
		WriteList("video_frames_badbeat", Sheet);
	}
}

// ------------------------------------------------------------------ kast_summary_cut

void KastCut()
{
	Desk D("rerun-kast");
	D.Fake("badbeat online"); // earlier in the day, before the stream
	D.S.LobbyMinutes = D.S.WorldMinutes() + 300.0;
	D.Fake("bluff online");
	D.Fake("bigpot skip");
	const double W = D.S.WorldMinutes();
	D.S.Stream.StartWorld = W - 60.0;
	D.S.Stream.Last = ss::kast::Summary();
	D.S.Stream.Last.Valid = true;
	D.S.Stream.Last.Minutes = 90.0;
	D.S.StreamCard = true;
	D.RL.OpenApp(ss::ui::RiverLine::App::Kast, D.Now);
	DrawList L = D.Frame();
	Check(DrewRun(L, "Cut highlights (2)"), "kast_summary_cut: the stream summary offers Cut highlights (N), tonight's two (§4.8)");
	Check(Overflows(L, {0.0f, 0.0f, 1600.0f, 962.0f}, D.M, "kast summary") == 0, "kast_summary_cut: the card's words fit");
	WriteList("kast_summary_cut", L);
	Check(ClickRun(D, L, "Cut highlights (2)"), "kast_summary_cut: press it");
	Check(D.RL.CurrentApp() == ss::ui::RiverLine::App::Video && !D.S.StreamCard, "kast_summary_cut: Rerun opens and the summary goes");
	L = D.Frame();
	Check(DrewRun(L, "TONIGHT'S STREAM") && DrewRun(L, "BLUFF") && DrewRun(L, "BIG POT") && !DrewRun(L, "BAD BEAT"), "kast_summary_cut: Studio filtered to tonight's stream");
	Check(Drew(L, "\xE2\x80\x9C"), "kast_summary_cut: tonight's best moment picked, its title in the editor");
	WriteList("video_studio_tonight", L);
	// Opened again later from the taskbar, the filter never cleared by hand: the whole list (review-ui-video: this used to
	// run after Show all, which had cleared the filter already).
	D.RL.OpenApp(ss::ui::RiverLine::App::Bank, D.Now);
	D.Frame();
	D.Now += 1.0;
	D.RL.OpenApp(ss::ui::RiverLine::App::Video, D.Now);
	L = D.Frame();
	Check(DrewRun(L, "BAD BEAT") && !DrewRun(L, "TONIGHT'S STREAM"), "kast_summary_cut: the filter doesn't outlive the visit");
	// Show all, from the filtered list (the summary's own window: the stream's start less a minute, its end plus one).
	D.Now += 1.0;
	ss::ui::rlvideo_detail::VideoOpenTonight(D.RL, W - 61.0, W + 31.0, D.Now);
	L = D.Frame();
	Check(DrewRun(L, "TONIGHT'S STREAM") && !DrewRun(L, "BAD BEAT"), "kast_summary_cut: filtered again");
	Check(ClickRun(D, L, "Show all"), "kast_summary_cut: Show all");
	L = D.Frame();
	Check(DrewRun(L, "BAD BEAT") && !DrewRun(L, "TONIGHT'S STREAM"), "kast_summary_cut: and the rest come back");
	// Everything from tonight already cut: the filtered list says so, and Show all is there.
	D.Now += 1.0;
	ss::ui::rlvideo_detail::VideoOpenTonight(D.RL, W + 600.0, W + 700.0, D.Now);
	L = D.Frame();
	Check(DrewRun(L, "Nothing left from tonight's stream.") && DrewRun(L, "Show all"), "kast_summary_cut: an empty night says so");
	// No moments tonight: no button.
	Desk E("rerun-kast-none");
	E.S.Stream.StartWorld = E.S.WorldMinutes() - 60.0;
	E.S.Stream.Last = ss::kast::Summary();
	E.S.Stream.Last.Valid = true;
	E.S.Stream.Last.Minutes = 60.0;
	E.S.StreamCard = true;
	E.RL.OpenApp(ss::ui::RiverLine::App::Kast, E.Now);
	L = E.Frame();
	Check(DrewRun(L, "Done") && !Drew(L, "Cut highlights"), "kast_summary_cut: no button on a stream with nothing to cut");
}

// ------------------------------------------------------------------ the bell and the taskbar's dot

void Bell()
{
	Desk D("rerun-bell");
	D.Fake("badbeat online");
	D.Fake("bluff online");
	D.Fake("bigpot online");
	D.Open(1);
	D.Frame();
	Check(!D.Dot(), "bell: no dot after a visit");
	// A milestone lands (core-video's Step writes the bell): the dot, and the header's count.
	D.RL.OpenApp(ss::ui::RiverLine::App::Bank, D.Now);
	D.Frame();
	D.S.Life.Video.Bell.push_back({D.S.WorldMinutes(), "100 subscribers. People are sticking around."});
	Check(D.Dot(), "bell: an unseen milestone lights the dot (§4.9)");
	D.Now += 1.0;
	D.Open(0);
	DrawList L = D.Frame();
	Check(DrewRun(L, "1"), "bell: the header's bell counts it");
	Check(D.Dot(), "bell: a visit alone leaves a milestone unseen until the bell is opened");
	D.Click(1600.0f - 44.0f, 30.0f);
	L = D.Frame();
	Check(DrewRun(L, "Milestones") && Drew(L, "100 subscribers. People are sticking around."), "bell: its list holds the milestone");
	Check(!D.Dot(), "bell: opened, the dot goes out");
	WriteList("video_bell", L);
	// A click anywhere else closes it, and does nothing under it.
	const size_t Picked = D.S.Life.Video.Videos.size();
	D.Click(800.0f, 700.0f);
	L = D.Frame();
	Check(!DrewRun(L, "Milestones") && D.S.Life.Video.Videos.size() == Picked && !D.S.TimeSkip.Active, "bell: a click off it closes it");
	Desk E("rerun-bell-empty");
	E.Open(0);
	E.Frame();
	E.Click(1600.0f - 44.0f, 30.0f);
	L = E.Frame();
	Check(DrewRun(L, "Nothing yet. Milestones land here as your videos grow."), "bell: honest when empty");
}

// ------------------------------------------------------------------ the laptop's widths

void Fits()
{
	Desk D("rerun-fit");
	for (const char* K : {"badbeat online", "bluff embercrest", "bigpot skip", "cooler dees", "rival online", "herofold online", "win online", "bubble online"})
	{
		D.Fake(K);
	}
	D.S.Life.Energy = 4.0; // a reason under PUBLISH, wrapped
	D.S.BankrollCents = 10000;
	D.Open(0);
	DrawList L = D.Frame();
	int Bad = 0;
	Bad += Overflows(L, {20.0f, 80.0f, 420.0f, 870.0f}, D.M, "studio list");
	Bad += Overflows(L, {460.0f, 80.0f, 700.0f, 870.0f}, D.M, "studio editor", ThumbBox);
	Bad += Overflows(L, {1180.0f, 80.0f, 400.0f, 470.0f}, D.M, "studio advice");
	Bad += Overflows(L, {0.0f, 0.0f, 1600.0f, 962.0f}, D.M, "studio");
	Check(Bad == 0, "fits: Studio's words keep inside their columns at the laptop's 1600 units");
	// The same layout at both screen sizes the acceptance shoots (§8.6: 1920 and 2552 wide): only the pixel scale changes.
	auto Runs = [](const DrawList& X) {
		std::vector<std::string> Out;
		for (const ss::ui::DrawCmd& Cmd : X.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
			{
				char Buf[64];
				std::snprintf(Buf, sizeof(Buf), "%.1f,%.1f,", Cmd.Text.X, Cmd.Text.BaselineY);
				Out.push_back(Buf + Cmd.Text.Text);
			}
		}
		return Out;
	};
	const std::vector<std::string> At1 = Runs(L);
	const std::vector<std::string> At1920 = Runs(D.Frame(-1.0f, -1.0f, false, false, false, 0.0f, 1920.0f / 1600.0f));
	const std::vector<std::string> At2552 = Runs(D.Frame(-1.0f, -1.0f, false, false, false, 0.0f, 2552.0f / 1600.0f));
	Check(At1 == At1920 && At1 == At2552, "fits: Studio lays out the same at 1920 and 2552 pixels wide");
	// The list scrolls when it's longer than the screen.
	D.S.DebugVideo("fake suckout online");
	D.S.DebugVideo("fake knockout online");
	D.S.DebugVideo("fake herocall online");
	D.S.DebugVideo("fake bluffcaught online");
	L = D.Frame();
	const VRun Top = RunOf(L, "BAD BEAT", D.M, 100.0f, 960.0f);
	for (int K = 0; K < 30; ++K)
	{
		D.Frame(200.0f, 500.0f, false, false, false, K < 4 ? 1.0f : 0.0f);
		D.Now += 1.0 / 30.0;
	}
	L = D.Frame(200.0f, 500.0f);
	const VRun After = RunOf(L, "BAD BEAT", D.M, -1.0e9f, 1.0e9f);
	Check(D.S.Life.Video.Open.size() >= 12 && Top.Found && (!After.Found || After.Base < Top.Base - 50.0f), "fits: the candidate list scrolls with the wheel");
	// The Channel, with a dozen videos and the gate half full.
	Desk C("rerun-fit-channel");
	C.S.DebugVideo("grant 480 2100 0");
	for (int I = 0; I < 9; ++I)
	{
		C.Rest();
		C.Fake(I % 3 == 0 ? "badbeat online" : I % 3 == 1 ? "bluff online" : "bigpot online");
		C.Publish(ChoiceOf({C.S.Life.Video.Open.back().Id}, static_cast<video::Format>(I % 2), static_cast<video::TitleStyle>(I % 5), static_cast<video::Frame>(I % 5)));
		C.S.DebugVideo("advance 2");
	}
	C.Open(1);
	L = C.Frame();
	Bad = 0;
	Bad += Overflows(L, {20.0f, 74.0f, 1560.0f, 136.0f}, C.M, "channel banner");
	Bad += Overflows(L, {20.0f, 226.0f, 600.0f, 256.0f}, C.M, "channel gate");
	Bad += Overflows(L, {640.0f, 226.0f, 940.0f, 256.0f}, C.M, "channel chart");
	Bad += Overflows(L, {0.0f, 0.0f, 1600.0f, 962.0f}, C.M, "channel");
	const float TileW = (1560.0f - 6.0f * 14.0f) / 7.0f;
	for (int Col = 0; Col < 7; ++Col)
	{
		Bad += Overflows(L, {20.0f + static_cast<float>(Col) * (TileW + 14.0f), 536.0f, TileW, 412.0f}, C.M, "channel tile");
	}
	Check(C.S.Life.Video.Videos.size() == 9 && Bad == 0, "fits: the Channel's words keep inside the banner, the cards and each tile");
	WriteList("video_channel_grid", L);
	// The measured note (§9): what a frame of each page costs to build (wall clock, this machine; printed, not checked).
	auto Ms = [](Desk& X, int Frames) {
		const auto T0 = std::chrono::steady_clock::now();
		for (int K = 0; K < Frames; ++K)
		{
			X.Frame();
		}
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / Frames;
	};
	D.Open(0);
	const double Studio = Ms(D, 40);
	const double Channel = Ms(C, 40);
	std::printf("  video draw: Studio %.2f ms (12 candidates), Channel %.2f ms (9 videos), the laptop's whole frame each\n", Studio, Channel);
}

// ------------------------------------------------------------------ review-ui-video's additions (2026-10-19)

/** A fixture's text (Tests/fixtures), "" when it can't be read. */
std::string VideoFixture(const std::string& Name)
{
	std::string Out;
	if (std::FILE* F = std::fopen((std::string(SS_TESTS_DIR) + "/fixtures/" + Name).c_str(), "rb"))
	{
		char Buf[4096];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Out.append(Buf, N);
		}
		std::fclose(F);
	}
	return Out;
}

/** A session that keeps what it last saved. */
struct SaveHooks : QuietHooks
{
	std::string Last;
	int Saves = 0;
	void Save(const ss::SaveData& Data) override
	{
		Last = Data.Serialize();
		++Saves;
	}
};

/** The colour of the first run that is exactly Text, and whether there was one. */
bool RunColour(const DrawList& L, const std::string& Text, ss::ui::Color& Out)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			Out = Cmd.Text.Col;
			return true;
		}
	}
	return false;
}

/** A text run starting inside Region that begins with Prefix. */
bool RunStartsIn(const DrawList& L, const ss::ui::Rect& Region, const std::string& Prefix)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Inside(Region, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY) && Cmd.Text.Text.rfind(Prefix, 0) == 0)
		{
			return true;
		}
	}
	return false;
}

/** Text runs inside Region that Canvas::Text cut short (an ellipsis): each printed, and how many. */
int Cut(const DrawList& L, const ss::ui::Rect& Region, const char* What, const ss::ui::Rect& Skip = {0.0f, 0.0f, 0.0f, 0.0f})
{
	int Bad = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Inside(Region, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY) && !(Skip.W > 0.0f && Inside(Skip, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY)) &&
			Cmd.Text.Text.find("\xE2\x80\xA6") != std::string::npos)
		{
			++Bad;
			std::printf("  %s: \"%s\" is cut short\n", What, Cmd.Text.Text.c_str());
		}
	}
	return Bad;
}

/**
 * An old save (§8.2 step 1, U10): Rerun opened, both pages, the bell, the tabs: the channel stays untouched, its line bit
 * stays clear, and the save it writes has no video line.
 */
void OldSave()
{
	ss::SaveData Data;
	const std::string Text = VideoFixture("pre_update2_sponsored_streamer.ssave");
	const bool Parsed = !Text.empty() && ss::SaveData::Parse(Text, Data);
	Check(Parsed && Text.find("\nvideo\t") == std::string::npos, "old save: pre_update2_sponsored_streamer reads, with no video line");
	if (!Parsed)
	{
		return;
	}
	SaveHooks H;
	ss::Session S(H, "rerun-old-save", &Data);
	S.CurrentScreen = ss::Screen::Lobby;
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	double Now = Step(S, 1.0, 0.2);
	const uint32_t BitsBefore = S.Life.Update2LinesRead;
	auto Frame = [&](float X, float Y, bool Pressed, bool Released) {
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Pressed;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		RL.UI.Ptr.Wheel = 0.0f;
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		Now += 0.05;
		return L;
	};
	auto Click = [&](float X, float Y) {
		Frame(X, Y, false, false);
		Frame(X, Y, true, false);
		return Frame(X, Y, false, true);
	};
	const bool Dot = ss::ui::rlvideo_detail::VideoDot(&RL, S);
	ss::ui::rlvideo_detail::VideoOpen(RL, 0, Now);
	DrawList L = Frame(-1.0f, -1.0f, false, false);
	const bool StudioEmpty = DrewRun(L, "Play some hands. The good ones show up here.");
	Click(1600.0f - 44.0f, 30.0f); // the bell
	L = Frame(-1.0f, -1.0f, false, false);
	const bool BellEmpty = DrewRun(L, "Nothing yet. Milestones land here as your videos grow.");
	Click(800.0f, 600.0f); // off the bell
	ss::ui::rlvideo_detail::VideoOpen(RL, 1, Now);
	L = Frame(-1.0f, -1.0f, false, false);
	const bool ChannelEmpty = DrewRun(L, "No videos yet.") && DrewRun(L, "0 / 1,000") && DrewRun(L, "0 / 4,000") && DrewRun(L, "Not yet");
	Click(800.0f, 300.0f); // the chart
	Click(211.0f, 30.0f);  // Studio's tab
	const ss::video::Channel& Ch = S.Life.Video;
	S.Save();
	const bool NoLines = H.Saves > 0 && H.Last.find("\nvideo\t") == std::string::npos && H.Last.rfind("video\t", 0) != 0;
	Check(!Dot && StudioEmpty && BellEmpty && ChannelEmpty, "old save: Rerun is empty and honest (no dot, Studio's and the Channel's empty states, the bell)");
	Check((S.Life.Update2LinesRead & ss::life::U2Video) == 0 && (BitsBefore & ss::life::U2Video) == 0 && Ch.Open.empty() && Ch.Videos.empty() && Ch.Bell.empty() && Ch.Subs == 0.0,
		"old save: looking at Rerun leaves the channel untouched, its line bit clear");
	Check(NoLines, "old save: the save it writes has no video line (U10: a channel never used writes nothing)");
}

/**
 * The partner gate crossed and under review (§4.6: a week's review): the bars full, UNDER REVIEW and its line whole, no
 * Partner mark, no money anywhere yet.
 */
void UnderReview()
{
	Desk D("rerun-review");
	D.Fake("badbeat online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Breakdown)), "review: a Breakdown goes up");
	D.S.DebugVideo("grant 1000 4000 0");
	D.S.DebugVideo("advance 2");
	const video::Channel& Ch = D.S.Life.Video;
	Check(Ch.ReviewAt >= 0.0 && Ch.PartnerAt < 0.0, "review: both bars full, the channel goes under review (not Partner yet)");
	D.Open(1);
	const DrawList L = D.Frame();
	Check(DrewRun(L, "UNDER REVIEW") && DrewRun(L, "Under review") && DrewRun(L, "Rerun's looking your channel over. Word comes by Mail in about a week."),
		"review: the gate card and the banner say so, the line whole");
	Check(!DrewRun(L, "PARTNER") && !DrewRun(L, "You're a Rerun Partner.") && !Drew(L, "earned") && !RunStartsIn(L, {640.0f, 420.0f, 940.0f, 62.0f}, "$"),
		"review: no Partner mark, no earnings and no revenue before the Partner Program says yes");
	Check(DrewRun(L, "Ads start when the review's done.") && !DrewRun(L, "Ads start at the gate."), "review: past the gate, the ads wait on the review (not \"at the gate\")");
	Check(Cut(L, {0.0f, 60.0f, 1600.0f, 422.0f}, "review") == 0, "review: nothing on the banner and the cards is cut short");
	WriteList("video_channel_review", L);
}

/** Views from before the gate never earn (§4.6): a tile posted before it counts only the views since. */
void EarnedSinceGate()
{
	Desk D("rerun-earned");
	D.Fake("badbeat online");
	Check(D.Publish(ChoiceOf({D.S.Life.Video.Open.back().Id}, video::Format::Breakdown)), "earned: a Breakdown before the gate");
	D.S.DebugVideo("advance 3");
	D.S.DebugVideo("grant 1000 4000 0");
	D.S.DebugVideo("advance 20");
	const video::Channel& Ch = D.S.Life.Video;
	const video::Video* Before = nullptr;
	for (const video::Video& X : Ch.Videos)
	{
		Before = X.At < Ch.PartnerAt && X.Title != "Granted views (dev)" ? &X : Before;
	}
	Check(Ch.PartnerAt >= 0.0 && Before != nullptr, "earned: Partner, with the video from before the gate");
	if (!Before || Ch.PartnerAt < 0.0)
	{
		return;
	}
	const double World = D.S.WorldMinutes();
	const double Since = std::max(0.0, video::Views(*Before, World) - video::Views(*Before, Ch.PartnerAt)) * Before->Rpm / 1000.0 * 100.0;
	const double All = video::Views(*Before, World) * Before->Rpm / 1000.0 * 100.0;
	const std::string SinceLine = ss::Money(static_cast<ss::Chips>(std::floor(Since))) + " earned";
	const std::string AllLine = ss::Money(static_cast<ss::Chips>(std::floor(All))) + " earned";
	D.Open(1);
	const DrawList L = D.Frame();
	std::printf("  video earned: a pre-gate video's tile reads \"%s\" (all its views would be \"%s\")\n", SinceLine.c_str(), AllLine.c_str());
	Check(SinceLine != AllLine && DrewRun(L, SinceLine) && !DrewRun(L, AllLine), "earned: the pre-gate video's tile counts only the views since the gate");
}

/** PUBLISH greyed is drawn greyed: its label in the disabled grey with a reason, in Rerun's ink when it can go. */
void PublishGreyed()
{
	Desk D("rerun-grey");
	D.Fake("badbeat online");
	D.Open(0);
	D.Frame();
	ss::ui::Color Ok, Tired, Choice;
	DrawList L = D.Frame();
	const bool HasOk = RunColour(L, "PUBLISH", Ok);
	D.S.Life.Energy = 5.0;
	L = D.Frame();
	const bool HasTired = RunColour(L, "PUBLISH", Tired);
	D.Rest();
	Check(ClickRun(D, L, "Vlog", 112.0f, 168.0f), "greyed: the Vlog format (one moment: not enough)");
	L = D.Frame();
	const bool HasChoice = RunColour(L, "PUBLISH", Choice);
	const ss::ui::Color Ink = ss::ui::Hex(ss::ui::videoart::Ink);
	Check(HasOk && SameColour(Ok, Ink), "greyed: PUBLISH in Rerun's ink on marigold when it can go");
	Check(HasTired && !SameColour(Tired, Ink) && HasChoice && !SameColour(Choice, Ink), "greyed: PUBLISH drawn greyed below 10 energy and for a choice that can't go up");
}

/** A Vlog holds eight moments (§4.5: 3 to 8 from one night): a ninth is refused, and Studio says so. */
void VlogFull()
{
	Desk D("rerun-vlog-full");
	const char* Kinds[9] = {"badbeat", "suckout", "cooler", "bluff", "bluffcaught", "herocall", "herofold", "bigpot", "knockout"};
	for (const char* K : Kinds)
	{
		D.Fake(std::string(K) + " online");
	}
	D.Open(0);
	DrawList L = D.Frame();
	Check(ClickRun(D, L, "Vlog", 112.0f, 168.0f), "vlog full: the Vlog format");
	const std::vector<video::Candidate> List = D.S.VideoCandidates();
	for (size_t I = 1; I < List.size(); ++I)
	{
		L = D.Frame();
		ClickRun(D, L, video::KindLabel(List[I].K), 100.0f, 960.0f);
	}
	L = D.Frame();
	Check(List.size() == 9 && DrewRun(L, "A Vlog holds eight moments at most.") && DrewRun(L, "8") && !DrewRun(L, "9"), "vlog full: eight picked, the ninth refused with a line");
}

/** A click off the open bell closes it and does nothing on the page under it. */
void BellSwallows()
{
	Desk D("rerun-bell-click");
	D.Fake("badbeat online");
	D.Open(0);
	DrawList L = D.Frame();
	Check(DrewRun(L, "Edit: 1 h 30 m \xC2\xB7 8 energy"), "bell click: a Breakdown to start");
	D.Click(1600.0f - 44.0f, 30.0f);
	L = D.Frame();
	Check(DrewRun(L, "Milestones"), "bell click: the bell is open");
	const VRun Vlog = RunOf(L, "Vlog", D.M, 112.0f, 168.0f);
	D.Click(Vlog.X + Vlog.W / 2.0f, Vlog.Base - 5.0f);
	L = D.Frame();
	Check(Vlog.Found && !DrewRun(L, "Milestones") && DrewRun(L, "Edit: 1 h 30 m \xC2\xB7 8 energy"), "bell click: the click closes the bell and doesn't pick the Vlog under it");
}

/**
 * The taskbar dot on a laptop that just came up (the last visit isn't saved: RiverLineVideo.cpp's VideoPrime): moments
 * from the last 12 world hours count as new, older ones as seen.
 */
void DotAfterLoad()
{
	Desk D("rerun-prime");
	D.Fake("badbeat online");
	D.Fake("bluff online");
	D.Fake("bigpot online");
	{
		ss::ui::RiverLine Fresh(D.S);
		Check(ss::ui::rlvideo_detail::VideoDot(&Fresh, D.S), "dot after a load: three moments from the last twelve hours light it");
	}
	D.S.LobbyMinutes = D.S.WorldMinutes() + 13.0 * 60.0;
	{
		ss::ui::RiverLine Later(D.S);
		Check(!ss::ui::rlvideo_detail::VideoDot(&Later, D.S), "dot after a load: moments older than twelve hours count as seen");
	}
}

/** A series event's long name gives way to the pot and who in the candidate's row (nothing important cut). */
void LongEventName()
{
	Desk D("rerun-long-name");
	D.Fake("badbeat online");
	D.Fake("win online");
	const std::string Long = "RLCOOP #23: $109 Headhunter High Roller (Turbo)";
	std::string Tail, WinTail;
	for (video::Candidate& C : D.S.Life.Video.Open)
	{
		C.Event = Long;
		if (C.K == video::Kind::BadBeat && C.Rec.Blinds.Bb > 0)
		{
			Tail = " \xC2\xB7 " + ss::Grouped(std::llround(static_cast<double>(C.Rec.Pot) / static_cast<double>(C.Rec.Blinds.Bb))) + " BB \xC2\xB7 vs " + C.Opponent;
		}
		if (C.K == video::Kind::Win)
		{
			WinTail = " \xC2\xB7 " + ss::Money(C.Rec.Pot);
		}
	}
	D.Open(0);
	const DrawList L = D.Frame();
	bool Beat = false, Win = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X < 140.0f && T.rfind("RLCOOP #23", 0) == 0)
		{
			Beat = Beat || (!Tail.empty() && T.size() > Tail.size() && T.compare(T.size() - Tail.size(), Tail.size(), Tail) == 0);
			Win = Win || (!WinTail.empty() && T.size() > WinTail.size() && T.compare(T.size() - WinTail.size(), WinTail.size(), WinTail) == 0);
		}
	}
	Check(!Tail.empty() && Beat && Win, "long event name: the row shortens the event's name and keeps the pot, who, and a run's prize");
	Check(Overflows(L, {20.0f, 80.0f, 420.0f, 870.0f}, D.M, "studio list") == 0, "long event name: the row's words keep inside the list");
}

/** The widest rows keep every chip: AUTO is the player's only sign the plan played (§1.10), so it never drops off. */
void ChipsFit()
{
	Desk D("rerun-chips");
	for (const char* K : {"bluffcaught skip", "finaltable skip", "herocall skip", "bluffcaught dees"})
	{
		D.Fake(K);
	}
	for (video::Candidate& C : D.S.Life.Video.Open)
	{
		C.Auto = true; // Dee's under the plan too (from night 2)
	}
	D.Open(0);
	DrawList L = D.Frame();
	Check(ClickRun(D, L, "Recap", 112.0f, 168.0f), "chips: a Recap (the rows' hints are the widest)");
	L = D.Frame();
	int Auto = 0, Site = 0, Story = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X < 440.0f)
		{
			Auto += Cmd.Text.Text == "AUTO" ? 1 : 0;
			Site += Cmd.Text.Text == "RIVERLINE" ? 1 : 0;
			Story += Cmd.Text.Text == "STORYTIME" ? 1 : 0;
		}
	}
	Check(D.S.Life.Video.Open.size() == 4 && Auto == 4 && Site == 3 && Story == 1, "chips: every row keeps AUTO, its site or STORYTIME beside the widest kinds");
}

// ------------------------------------------------------------------ the copy

void Copy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	for (const std::string& S : ss::ui::rlvideo_detail::VideoUiInventory())
	{
		Lines.push_back({"rerun", S});
	}
	Failures += copylint::Run("ui-video", Lines, copylint::LetterBody);
	++Checks;
	// In the world's words nothing ever says the plan played (U1), and no line names a real platform (§1.8).
	int Bad = 0;
	for (const auto& L : Lines)
	{
		// Word by word, so "both" isn't "bot".
		std::vector<std::string> Words(1);
		for (const char C : L.second)
		{
			if ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z'))
			{
				Words.back().push_back(static_cast<char>(C >= 'A' && C <= 'Z' ? C - 'A' + 'a' : C));
			}
			else if (!Words.back().empty())
			{
				Words.emplace_back();
			}
		}
		for (const std::string& W : Words)
		{
			for (const char* Banned : {"youtube", "twitch", "tiktok", "rumble", "vimeo", "dailymotion", "kick", "bot", "bots", "ai", "autoplay"})
			{
				Bad += W == Banned ? 1 : 0;
			}
		}
	}
	Check(Bad == 0, "copy: Rerun's lines never name a real platform and never call the plan a bot");
}
} // namespace video_seg

void SegVideo()
{
	video_seg::Empty();
	video_seg::StudioFlow();
	video_seg::PublishReasons();
	video_seg::Formats();
	video_seg::Storytime();
	video_seg::ChannelGate();
	video_seg::ChannelPartner();
	video_seg::Frames();
	video_seg::KastCut();
	video_seg::Bell();
	video_seg::Fits();
	video_seg::OldSave();
	video_seg::UnderReview();
	video_seg::EarnedSinceGate();
	video_seg::PublishGreyed();
	video_seg::VlogFull();
	video_seg::BellSwallows();
	video_seg::DotAfterLoad();
	video_seg::LongEventName();
	video_seg::ChipsFit();
	video_seg::Copy();
	std::printf("  ui-video: %d checks\n", video_seg::Checks);
}
} // namespace ui_test
