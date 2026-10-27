// rivals_ui: Phase A's UI seams for the revamped sites (update 3 SPEC §1.2, §1.11, §7.2, §8.2), the lead's file. The motion
// kit's closed forms; the font seam (Canvas::Measure(S, Size, Font), TextStyle::Face, the face index in the JSON, the
// Ui wrapper); GlowPolygon and StrokeDashed; house law 3's SitesBlockUnder (a one-frame click under an overlay fires nothing,
// a scrim click is swallowed); the shared two presses (SitesTwoPress); SitesPageHome's remap rule; SitesTableStrip; and
// ss.Laptop.Open's DebugOpen. RiverLine's draw lists through ClientFrame and SiteLook are proven identical to the day-0 JSON
// outside the suite (ui_test <dir> and the Kast preview harness against u3-day0). Compiled into rivals_ui_test and called
// from rivals_ui_main.cpp; the helpers go in ui_test::seams_seg.
#include "UiSeg.h"

#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/SiteMotion.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ui_test
{
namespace seams_seg
{
using ss::ui::Canvas;
using ss::ui::DrawCmd;
using ss::ui::DrawList;

/** The vertices' bounding box of a draw list's triangles whose alpha is (Clear) or isn't (!Clear) zero. */
ss::ui::Rect Bounds(const DrawList& L, bool Clear)
{
	float X0 = 1e9f, Y0 = 1e9f, X1 = -1e9f, Y1 = -1e9f;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if ((V.Col.A == 0.0f) != Clear)
		{
			continue;
		}
		X0 = std::min(X0, V.X);
		Y0 = std::min(Y0, V.Y);
		X1 = std::max(X1, V.X);
		Y1 = std::max(Y1, V.Y);
	}
	return {X0, Y0, X1 - X0, Y1 - Y0};
}

bool Near(float A, float B, float Tol = 0.02f)
{
	return std::fabs(A - B) <= Tol;
}

void Motion()
{
	namespace mo = ss::ui::motion;
	// CountUp: From at At, To at At + Seconds and after, eased out (past halfway by half time), never past To.
	Expect(mo::CountUp(0.0, 610.0, 5.0, 5.0, 0.8) == 0.0 && mo::CountUp(0.0, 610.0, 4.0, 5.0, 0.8) == 0.0, "CountUp starts at From");
	Expect(mo::CountUp(0.0, 610.0, 5.8, 5.0, 0.8) == 610.0 && mo::CountUp(0.0, 610.0, 99.0, 5.0, 0.8) == 610.0, "CountUp lands on To and stays");
	Expect(mo::CountUp(0.0, 610.0, 5.4, 5.0, 0.8) > 305.0, "CountUp eases out");
	{
		double Prev = -1.0;
		bool Rising = true;
		for (int K = 0; K <= 40; ++K)
		{
			const double V = mo::CountUp(100.0, 900.0, 2.0 + K * 0.025, 2.0, 1.0);
			Rising = Rising && V >= Prev && V <= 900.0;
			Prev = V;
		}
		Expect(Rising, "CountUp only climbs, and never past To");
	}
	Expect(mo::CountUp(5.0, 9.0, 0.0, 1.0, 0.0) == 9.0, "a zero-length count is already there");
	// Stage: how many marks the clock has passed.
	const std::vector<double> Marks = {0.0, 0.4, 1.1};
	Expect(mo::Stage(0.9, 1.0, Marks) == 0 && mo::Stage(1.0, 1.0, Marks) == 1 && mo::Stage(1.5, 1.0, Marks) == 2 && mo::Stage(9.0, 1.0, Marks) == 3, "Stage counts the marks passed");
	// Spring: 0 at At, overshoots a little, exactly 1 once settled.
	double Peak = 0.0;
	for (int K = 0; K < 150; ++K)
	{
		Peak = std::max(Peak, mo::Spring(K * 0.01, 0.0));
	}
	Expect(mo::Spring(0.0, 0.0) == 0.0 && mo::Spring(-1.0, 0.0) == 0.0, "Spring starts at 0");
	Expect(Peak > 1.0 && Peak < 1.15, "Spring overshoots a little");
	Expect(mo::Spring(1.5, 0.0) == 1.0 && mo::Spring(30.0, 0.0) == 1.0, "Spring settles on exactly 1 (a still never wobbles)");
	// Shake: still before and after, bounded, its direction the Id's.
	const ss::ui::Vec2 S0 = mo::Shake(0.0, 0.0, 7u), S1 = mo::Shake(0.6, 0.0, 7u), Sa = mo::Shake(0.03, 0.0, 7u), Sb = mo::Shake(0.03, 0.0, 8u);
	Expect(S0.X == 0.0f && S0.Y == 0.0f && S1.X == 0.0f && S1.Y == 0.0f, "Shake is still before At and once it's done");
	Expect(std::hypot(Sa.X, Sa.Y) > 0.5f && std::hypot(Sa.X, Sa.Y) <= 8.0f, "Shake moves, at most 8 units");
	Expect(std::fabs(Sa.X - Sb.X) + std::fabs(Sa.Y - Sb.Y) > 0.01f, "Shake's direction is hashed from its id");
	Expect(mo::Shake(0.03, 0.0, 7u).X == Sa.X, "Shake is a closed form (the same clock, the same offset)");
	// Flap: no characters before 0.25 s, then one every 50 ms, never more than the row.
	Expect(mo::Flap(10.2, 10.0, 12) == 0 && mo::Flap(10.25, 10.0, 12) == 1 && mo::Flap(10.36, 10.0, 12) == 3 && mo::Flap(20.0, 10.0, 12) == 12, "Flap settles a row left to right");
	// Chase and Beat: bounded; every third bulb lit, stepping a bulb a beat.
	bool Bounded = true;
	int Lit = 0;
	for (int I = 0; I < 30; ++I)
	{
		const double B = mo::Chase(10.0 + 0.19, I, 30, 120.0);
		Bounded = Bounded && B >= 0.0 && B <= 1.0;
		Lit += B > 0.9 ? 1 : 0;
	}
	Expect(Bounded && Lit == 10, "Chase lights every third bulb");
	Expect(mo::Chase(0.25, 0, 30, 60.0) > 0.9 && mo::Chase(0.25, 1, 30, 60.0) < 0.9 && mo::Chase(1.25, 1, 30, 60.0) > 0.9 && mo::Chase(1.25, 0, 30, 60.0) < 0.9, "Chase steps one bulb a beat");
	Expect(mo::Beat(0.25, 120.0) > 0.49 && mo::Beat(0.25, 120.0) < 0.51 && mo::Beat(0.5, 120.0) < 1e-9 && mo::Beat(3.0, 0.0) == 0.0, "Beat is the phase within the beat");

	// Confetti: nothing before At or after its run, at most 200 pieces, the same frame for the same clock, and the parent's
	// alpha (it never sets its own).
	TableMeasurer M;
	auto Draw = [&](uint32_t Id, double Now, int N, float Alpha) {
		DrawList L;
		Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		C.SetAlpha(Alpha);
		mo::Confetti(C, Id, 10.0, Now, N, {300.0f, 100.0f, 800.0f, 600.0f});
		Expect(C.GetAlpha() == Alpha, "Confetti leaves the canvas's alpha as it found it");
		return L;
	};
	Expect(Draw(1u, 9.9, 120, 1.0f).Vertices.empty() && Draw(1u, 10.0, 120, 1.0f).Vertices.empty() && Draw(1u, 12.7, 120, 1.0f).Vertices.empty(), "Confetti draws nothing outside its run");
	const DrawList Mid = Draw(1u, 10.5, 120, 1.0f);
	Expect(!Mid.Vertices.empty() && Mid.Vertices.size() <= 120 * 9, "Confetti draws its pieces (a quad each)");
	Expect(Draw(1u, 10.5, 1000, 1.0f).Vertices.size() <= 200 * 9, "Confetti never draws more than 200 quads");
	Expect(Draw(1u, 10.5, 120, 1.0f).ToJson() == Mid.ToJson(), "Confetti is a closed form of Now - At");
	Expect(Draw(2u, 10.5, 120, 1.0f).ToJson() != Mid.ToJson(), "Confetti's paths are hashed from its id");
	{
		const DrawList Half = Draw(1u, 10.5, 120, 0.5f);
		bool Scaled = Half.Vertices.size() == Mid.Vertices.size();
		for (size_t I = 0; Scaled && I < Mid.Vertices.size(); ++I)
		{
			Scaled = Near(Half.Vertices[I].Col.A, Mid.Vertices[I].Col.A * 0.5f, 1e-4f);
		}
		Expect(Scaled, "Confetti multiplies the parent's alpha");
	}
	std::printf("  seams motion   CountUp, Stage, Spring (peak %.3f), Shake, Flap, Chase, Beat, Confetti (%zu vertices at 0.5 s)\n", Peak, Mid.Vertices.size());
}

void Fonts()
{
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	const std::string Word = "LIGHT IT";
	float Sum = 0.0f;
	for (const char Ch : Word)
	{
		Sum += test_font::BlackItalicAscii[Ch - 32];
	}
	Expect(Near(C.Measure(Word, 40.0f, ss::ui::Font::BlackItalic), Sum * 0.4f, 1e-3f), "Measure(S, Size, Font) measures the face drawn");
	Expect(C.Measure(Word, 40.0f, ss::ui::Font::BlackItalic) != C.Measure(Word, 40.0f, 900), "BlackItalic isn't measured as Black");
	Expect(C.Measure("BOARD", 12.0f, ss::ui::Font::BoldCondensed) < C.Measure("BOARD", 12.0f, 700), "BoldCondensed measures narrower than Bold");
	Expect(C.Measure(Word, 18.0f, ss::ui::Font::Regular) == C.Measure(Word, 18.0f, 500), "the weights' faces measure as before");
	ss::ui::Ui U;
	U.Begin(C, 0.0);
	Expect(U.Measure("BOARD", 12.0f, ss::ui::Font::BoldCondensed) == C.Measure("BOARD", 12.0f, ss::ui::Font::BoldCondensed), "Ui's Measure(S, Size, Font) is the canvas's");
	U.End();
	// TextStyle::Face draws (and truncates, and aligns) in its own face; -1 keeps the weight's.
	ss::ui::TextStyle St = ss::ui::Ts(30.0f, 900, ss::ui::Hex(0xffffff), ss::ui::Align::Right);
	St.Face = static_cast<int8_t>(ss::ui::Font::BlackItalic);
	const float Wd = C.Text(Word, 500.0f, 100.0f, St);
	Expect(!L.Cmds.empty() && L.Cmds.back().Text.Face == ss::ui::Font::BlackItalic, "TextStyle::Face draws BlackItalic");
	Expect(Near(Wd, C.Measure(Word, 30.0f, ss::ui::Font::BlackItalic), 1e-3f) && Near(L.Cmds.back().Text.X, 500.0f - Wd, 1e-3f), "a face's text right-aligns by its own width");
	St.Face = static_cast<int8_t>(ss::ui::Font::BoldCondensed);
	St.MaxWidth = 60.0f;
	St.HAlign = ss::ui::Align::Left;
	const float Cut = C.Text("SETTLEMENT OF THE NIGHTCAP", 20.0f, 200.0f, St);
	Expect(Cut <= 60.0f && L.Cmds.back().Text.Face == ss::ui::Font::BoldCondensed, "a face's text truncates by its own width");
	C.Text("Regular", 20.0f, 300.0f, ss::ui::Ts(18.0f));
	Expect(L.Cmds.back().Text.Face == ss::ui::Font::Regular, "a style without a face draws its weight's");
	const std::string J = L.ToJson();
	Expect(J.find("\"font\":5") != std::string::npos && J.find("\"font\":6") != std::string::npos && J.find("\"font\":0") != std::string::npos, "the JSON carries the face index (5 BlackItalic, 6 BoldCondensed)");
	Expect(Near(M.Ascent(ss::ui::Font::BlackItalic, 100.0f), test_font::BlackItalicAscent) && Near(M.Ascent(ss::ui::Font::BoldCondensed, 100.0f), test_font::BoldCondensedAscent), "the test measurer knows both faces");
}

void GlowAndDashes()
{
	TableMeasurer M;
	const ss::ui::Color Teal = ss::ui::Rgba(60, 230, 210, 0.8f);
	// GlowPolygon: GlowRoundRect's reach (Blur past the outline), a solid core inset by a third of it; either winding.
	for (int Wind = 0; Wind < 2; ++Wind)
	{
		DrawList L;
		Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		std::vector<ss::ui::Vec2> Sq = {{100.0f, 100.0f}, {300.0f, 100.0f}, {300.0f, 200.0f}, {100.0f, 200.0f}};
		if (Wind == 1)
		{
			std::reverse(Sq.begin(), Sq.end());
		}
		C.GlowPolygon(Sq, Teal, 20.0f);
		const ss::ui::Rect Out = Bounds(L, true);
		const ss::ui::Rect Core = Bounds(L, false);
		Expect(L.Vertices.size() == 9, "a glowing quad is one feathered path (2N + 1 vertices)");
		Expect(Near(Out.X, 80.0f, 0.05f) && Near(Out.Y, 80.0f, 0.05f) && Near(Out.W, 240.0f, 0.1f) && Near(Out.H, 140.0f, 0.1f), "the glow reaches Blur past the outline");
		Expect(Near(Core.X, 106.0f, 0.05f) && Near(Core.W, 188.0f, 0.1f) && Near(Core.H, 88.0f, 0.1f), "the glow's core sits a third of Blur inside");
	}
	{
		// A zero blur is the polygon itself; a canvas's scale widens the glow with it.
		DrawList A, B;
		Canvas Ca(A, M, 1600.0f, 1000.0f, 1.0f), Cb(B, M, 1600.0f, 1000.0f, 1.0f);
		const std::vector<ss::ui::Vec2> Tri = {{10.0f, 10.0f}, {90.0f, 20.0f}, {40.0f, 80.0f}};
		Ca.GlowPolygon(Tri, Teal, 0.0f);
		Cb.FillPolygon(Tri, Teal);
		Expect(A.ToJson() == B.ToJson(), "GlowPolygon with no blur is FillPolygon");
		DrawList S;
		Canvas Cs(S, M, 1600.0f, 1000.0f, 1.0f);
		Cs.Scale(2.0f, 2.0f);
		Cs.GlowPolygon({{100.0f, 100.0f}, {300.0f, 100.0f}, {300.0f, 200.0f}, {100.0f, 200.0f}}, Teal, 20.0f);
		Expect(Near(Bounds(S, true).X, 160.0f, 0.1f), "the glow scales with the canvas");
	}
	// StrokeDashed: Dash on, Gap off, along the line; a dash keeps a corner it turns; Dash or Gap <= 0 is solid.
	{
		DrawList L;
		Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		C.StrokeDashed({{0.0f, 50.0f}, {100.0f, 50.0f}}, Teal, 2.0f, 10.0f, 5.0f);
		Expect(L.Vertices.size() == 7 * 8, "100 units at 10 on, 5 off is seven dashes");
		bool Placed = L.Vertices.size() == 56;
		for (size_t K = 0; Placed && K < 7; ++K)
		{
			const float X0 = L.Vertices[K * 8 + 1].X;
			const float X1 = L.Vertices[K * 8 + 5].X;
			Placed = Near(X0, 15.0f * static_cast<float>(K), 1e-3f) && Near(X1, std::min(100.0f, 15.0f * static_cast<float>(K) + 10.0f), 1e-3f);
		}
		Expect(Placed, "each dash runs Dash units, Gap apart");
	}
	{
		DrawList L;
		Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		// 12 on: 8 along, the corner, 4 down (three points); 4 off; the last 12 down to the end (two points).
		C.StrokeDashed({{0.0f, 0.0f}, {8.0f, 0.0f}, {8.0f, 20.0f}}, Teal, 2.0f, 12.0f, 4.0f);
		Expect(L.Vertices.size() == 12 + 8 && Near(L.Vertices[9].Y, 4.0f, 1e-3f) && Near(L.Vertices[13].Y, 8.0f, 1e-3f), "a dash that turns a corner keeps it (three points)");
	}
	{
		DrawList A, B;
		Canvas Ca(A, M, 1600.0f, 1000.0f, 1.0f), Cb(B, M, 1600.0f, 1000.0f, 1.0f);
		const std::vector<ss::ui::Vec2> Pts = {{0.0f, 0.0f}, {50.0f, 30.0f}, {90.0f, 10.0f}};
		Ca.StrokeDashed(Pts, Teal, 3.0f, 0.0f, 5.0f);
		Cb.StrokePolyline(Pts, false, Teal, 3.0f);
		Expect(A.ToJson() == B.ToJson(), "StrokeDashed with no dash is StrokePolyline");
		DrawList S;
		Canvas Cs(S, M, 1600.0f, 1000.0f, 1.0f);
		Cs.Scale(2.0f, 2.0f);
		Cs.StrokeDashed({{0.0f, 50.0f}, {100.0f, 50.0f}}, Teal, 2.0f, 10.0f, 5.0f);
		Expect(S.Vertices.size() == 7 * 8 && Near(S.Vertices[8 + 1].X, 30.0f, 1e-3f), "the dashes scale with the canvas");
	}
}

/** One frame of a Ui on its own canvas with the pointer at (X, Y): Press and Release in the same frame is a one-frame click. */
struct Rig
{
	TableMeasurer M;
	DrawList L;
	Canvas C{L, M, 1600.0f, 1000.0f, 1.0f};
	ss::ui::Ui U;
	void Begin(float X, float Y, bool Press, bool Release)
	{
		L.Clear();
		U.Ptr.Active = true;
		U.Ptr.X = X;
		U.Ptr.Y = Y;
		U.Ptr.Pressed = Press;
		U.Ptr.Released = Release;
		U.Ptr.Down = Press && !Release;
		U.Begin(C, 1.0);
	}
	void End() { U.End(); }
};

void BlockUnder()
{
	namespace sd = ss::ui::rlsites_detail;
	const ss::ui::Rect Sheet{400.0f, 300.0f, 800.0f, 400.0f};
	const ss::ui::Rect Under{100.0f, 100.0f, 200.0f, 60.0f}; // a live button outside the sheet
	const ss::ui::Rect Beneath{500.0f, 400.0f, 200.0f, 60.0f}; // and one the sheet covers
	Rig R;
	// A modal overlay: a one-frame click on a button under it (anywhere) fires nothing; the overlay's own button does.
	R.Begin(600.0f, 430.0f, true, true);
	sd::SitesBlocked B = sd::SitesBlockUnder(R.U, Sheet, true);
	const bool Fired = R.U.Button("beneath", Beneath, "Under");
	sd::SitesUnblock(R.U, B);
	const bool Own = R.U.Button("sheetok", Beneath, "Over");
	R.End();
	Expect(B.Active && !B.Scrim && !Fired && Own, "a one-frame click on the overlay reaches the overlay, never the button under it");
	R.Begin(200.0f, 130.0f, true, true);
	B = sd::SitesBlockUnder(R.U, Sheet, true);
	const bool Outside = R.U.Button("under", Under, "Under");
	sd::SitesUnblock(R.U, B);
	const bool AfterScrim = R.U.Button("taskbar", Under, "After");
	R.End();
	Expect(B.Scrim && !Outside && !AfterScrim, "a click on the scrim closes the overlay and is swallowed (nothing under or after it fires)");
	// Not modal (the dock's pill): only what's under R is held back.
	R.Begin(200.0f, 130.0f, true, true);
	B = sd::SitesBlockUnder(R.U, Sheet, false);
	const bool Free = R.U.Button("under", Under, "Under");
	sd::SitesUnblock(R.U, B);
	R.End();
	Expect(!B.Active && !B.Scrim && Free, "a non-modal block leaves the rest of the screen live");
	R.Begin(600.0f, 430.0f, true, true);
	B = sd::SitesBlockUnder(R.U, Sheet, false);
	const bool Held = R.U.Button("beneath", Beneath, "Under");
	sd::SitesUnblock(R.U, B);
	R.End();
	Expect(B.Active && !Held, "a non-modal block holds back what's under its rect");
}

void TwoPresses()
{
	namespace sd = ss::ui::rlsites_detail;
	const sd::SiteLook& Look = sd::SitesLook(ss::net::SiteId::RiverLine);
	const ss::ui::Rect R{400.0f, 500.0f, 400.0f, 70.0f};
	auto Clicked = [&](Rig& G, float X, const std::string& Id, const std::string& Label, bool Clause, const sd::TwoPressOpts& O) {
		G.Begin(X, R.Y + 30.0f, true, true);
		const sd::TwoPress P = sd::SitesTwoPress(G.C, G.U, Look, Id, R, Label, Clause, O);
		G.End();
		return P;
	};
	auto Idle = [&](Rig& G, const std::string& Id, const std::string& Label, bool Clause, const sd::TwoPressOpts& O) {
		G.Begin(-50.0f, -50.0f, false, false);
		const sd::TwoPress P = sd::SitesTwoPress(G.C, G.U, Look, Id, R, Label, Clause, O);
		G.End();
		return P;
	};
	{
		// Off the clause, one press goes (FUSE's LIGHT IT, a cabinet).
		Rig G;
		sd::TwoPressOpts O;
		Expect(Idle(G, "fuse0", "LIGHT IT", false, O) == sd::TwoPress::Idle && Drew(G.L, "LIGHT IT"), "the button waits");
		Expect(Clicked(G, R.X + 100.0f, "fuse0", "LIGHT IT", false, O) == sd::TwoPress::Confirmed, "off the clause one press confirms");
	}
	{
		// Under the clause: the warning, then "Take the strike" or "Cancel"; the state is kept by button, per laptop.
		Rig G;
		sd::TwoPressOpts O;
		O.Strike = "Strike under your Team RiverLine deal";
		Idle(G, "fuse1", "LIGHT IT", true, O);
		Expect(Drew(G.L, "Strike under your Team RiverLine deal") && Drew(G.L, "LIGHT IT anyway"), "under the clause the button says it's a strike");
		Expect(Clicked(G, R.X + 100.0f, "fuse1", "LIGHT IT", true, O) == sd::TwoPress::Armed, "the first press arms");
		Idle(G, "fuse1", "LIGHT IT", true, O);
		Expect(Drew(G.L, ss::ui::rlsites_detail::SitesStrikeConfirm) && Drew(G.L, "Take the strike") && Drew(G.L, "Cancel"), "armed, it asks once more");
		Expect(Idle(G, "fuse2", "LIGHT IT", true, O) == sd::TwoPress::Idle, "another button isn't armed by it");
		Expect(Clicked(G, R.X + 300.0f, "fuse1", "LIGHT IT", true, O) == sd::TwoPress::Cancelled && Idle(G, "fuse1", "LIGHT IT", true, O) == sd::TwoPress::Idle, "Cancel disarms");
		Clicked(G, R.X + 100.0f, "fuse1", "LIGHT IT", true, O);
		Expect(Clicked(G, R.X + 100.0f, "fuse1", "LIGHT IT", true, O) == sd::TwoPress::Confirmed && Idle(G, "fuse1", "LIGHT IT", true, O) == sd::TwoPress::Idle, "the second press takes the strike, once");
	}
	{
		// RegisterBlock's: a confirm step off the clause too, its state the caller's (Session::ConfirmRegister).
		Rig G;
		bool Armed = false;
		sd::TwoPressOpts O;
		O.First.Kind = ss::ui::ButtonKind::Primary;
		O.First.Size = 20.0f;
		O.Note = "Balance after: $12.00";
		O.Always = true;
		O.Armed = &Armed;
		Expect(Clicked(G, R.X + 100.0f, "reg", "Register", false, O) == sd::TwoPress::Armed && Armed, "the caller's state arms");
		Idle(G, "reg", "Register", false, O);
		Expect(Drew(G.L, "Balance after: $12.00") && Drew(G.L, "Confirm") && !Drew(G.L, "Take the strike"), "the confirm step says what the entry leaves");
		Expect(Clicked(G, R.X + 100.0f, "reg", "Register", false, O) == sd::TwoPress::Confirmed && !Armed, "Confirm confirms and disarms");
	}
}

void PageHome()
{
	using Pg = ss::ui::RiverLine::Page;
	using ss::net::SiteId;
	namespace sd = ss::ui::rlsites_detail;
	const Pg Shared[4] = {Pg::Lobby, Pg::Series, Pg::Leaderboards, Pg::News};
	bool Ok = true;
	for (const SiteId X : {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice})
	{
		for (const Pg P : Shared)
		{
			Ok = Ok && sd::SitesPageHome(X, P) == P;
		}
	}
	Expect(Ok, "every client keeps the shared pages");
	Expect(sd::SitesPageHome(SiteId::RiverLine, Pg::Career) == Pg::Career && sd::SitesPageHome(SiteId::RiverLine, Pg::Embercrest) == Pg::Embercrest && sd::SitesPageHome(SiteId::RiverLine, Pg::Backing) == Pg::Backing,
		"RiverLine keeps its own pages");
	Expect(sd::SitesPageHome(SiteId::Ignite, Pg::Career) == Pg::Lobby && sd::SitesPageHome(SiteId::RunItTwice, Pg::Backing) == Pg::Lobby && sd::SitesPageHome(SiteId::Ignite, Pg::Embercrest) == Pg::Lobby,
		"RiverLine's pages go to a rival's Lobby");
	Expect(sd::SitesPageHome(SiteId::RiverLine, Pg::Promotions) == Pg::Lobby && sd::SitesPageHome(SiteId::RiverLine, Pg::Cashier) == Pg::Lobby && sd::SitesPageHome(SiteId::Ignite, Pg::Cashier) == Pg::Cashier,
		"a rival's pages go to RiverLine's Lobby, and stay on a rival");
	Expect(sd::SitesPageHome(SiteId::Ignite, Pg::Fuse) == Pg::Fuse && sd::SitesPageHome(SiteId::Ignite, Pg::Profile) == Pg::Profile && sd::SitesPageHome(SiteId::RunItTwice, Pg::Fuse) == Pg::Lobby &&
			   sd::SitesPageHome(SiteId::RiverLine, Pg::Fuse) == Pg::Lobby,
		"Ignite's own pages stay Ignite's");
	Expect(sd::SitesPageHome(SiteId::RunItTwice, Pg::League) == Pg::League && sd::SitesPageHome(SiteId::RunItTwice, Pg::Live) == Pg::Live && sd::SitesPageHome(SiteId::RunItTwice, Pg::Desk) == Pg::Desk &&
			   sd::SitesPageHome(SiteId::Ignite, Pg::League) == Pg::Lobby && sd::SitesPageHome(SiteId::RiverLine, Pg::Desk) == Pg::Lobby,
		"RunItTwice's own pages stay RunItTwice's");
}

void OpenAndStrip()
{
	using Pg = ss::ui::RiverLine::Page;
	using App = ss::ui::RiverLine::App;
	QuietHooks H;
	ss::Session S(H, "u3-seams-open");
	S.CurrentScreen = ss::Screen::Lobby;
	ss::ui::RiverLine RL(S);
	Expect(RL.DebugOpen("riverline series") && RL.CurrentApp() == App::RiverLine && RL.CurrentPage() == Pg::Series, "ss.Laptop.Open riverline series");
	Expect(RL.DebugOpen("RiverLine League RunItTwice") && RL.CurrentPage() == Pg::League && RL.SiteShown == ss::net::SiteId::RunItTwice, "a page and a site, any case");
	Expect(RL.DebugOpen("riverline ignite") && RL.SiteShown == ss::net::SiteId::Ignite && RL.CurrentPage() == Pg::Lobby, "a site alone opens its lobby");
	Expect(RL.DebugOpen("kast channel") && RL.CurrentApp() == App::Kast && RL.CurrentKastPage() == ss::ui::RiverLine::KastPage::Channel, "ss.Laptop.Open kast channel");
	Expect(RL.DebugOpen("video studio") && RL.CurrentApp() == App::Video, "ss.Laptop.Open video studio");
	Expect(RL.DebugOpen("bank") && RL.CurrentApp() == App::Bank && RL.DebugOpen("burner sam") && RL.CurrentApp() == App::Burner, "the other apps");
	Expect(!RL.DebugOpen("") && !RL.DebugOpen("riverline nowhere") && !RL.DebugOpen("bank vault") && !RL.DebugOpen("video nowhere") && RL.CurrentApp() == App::Burner,
		"a name that names nothing opens nothing");
	// C3 (Rerun 2.0): Analytics and Home are pages now, so ss.Laptop.Open names them.
	Expect(RL.DebugOpen("video analytics") && RL.CurrentApp() == App::Video && RL.DebugOpen("video home") && RL.CurrentApp() == App::Video, "ss.Laptop.Open video analytics, video home");
	// SitesTableStrip: the open tables in order (none here).
	Expect(ss::ui::rlsites_detail::SitesTableStrip(RL, S).empty(), "no tables, no strip");
}
} // namespace seams_seg

void SegSeams()
{
	seams_seg::Motion();
	seams_seg::Fonts();
	seams_seg::GlowAndDashes();
	seams_seg::BlockUnder();
	seams_seg::TwoPresses();
	seams_seg::PageHome();
	seams_seg::OpenAndStrip();
}
} // namespace ui_test
