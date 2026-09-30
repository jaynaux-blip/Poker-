// Renders the RiverLine client and the phone screen into draw lists for a set
// of game states and writes them as JSON. web/scripts/render-drawlists.mjs
// replays them in Chromium so the C++ UI can be compared with the prototype.
// Usage: ui_test <out-dir>   (with no argument it only checks the frames draw)
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/RiverLine.h"
#include "TestFontMetrics.h"

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

/** Text widths from the Chromium metrics table (the fonts the prototype renders with in CI). */
struct TableMeasurer : ss::ui::TextMeasurer
{
	static const float* Ascii(ss::ui::Font F)
	{
		return F == ss::ui::Font::Bold ? test_font::BoldAscii : F == ss::ui::Font::Mono ? test_font::MonoAscii : test_font::RegularAscii;
	}
	static const float* Special(ss::ui::Font F)
	{
		return F == ss::ui::Font::Bold ? test_font::BoldSpecial : F == ss::ui::Font::Mono ? test_font::MonoSpecial : test_font::RegularSpecial;
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
		const float A = Face == ss::ui::Font::Bold ? test_font::BoldAscent : Face == ss::ui::Font::Mono ? test_font::MonoAscent : test_font::RegularAscent;
		return A * SizePx / 100.0f;
	}
};

struct QuietHooks : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Texts;
	void Text(const std::string& From, const std::string& Body) override { Texts.push_back({From, Body}); }
};

void Emit(const std::string& Name, ss::ui::RiverLine& RL, double Now)
{
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
	RL.Draw(C, Now);
	Expect(!L.Cmds.empty() && L.Vertices.size() > 100, "frame drew something");
	size_t Texts = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Texts += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	std::printf("  %-14s %6zu vertices %6zu triangles %4zu text runs %4zu commands\n", Name.c_str(), L.Vertices.size(), L.Indices.size() / 3, Texts, L.Cmds.size());
	if (!OutDir.empty())
	{
		const std::string Path = OutDir + "/" + Name + ".json";
		if (FILE* F = std::fopen(Path.c_str(), "wb"))
		{
			const std::string J = L.ToJson();
			std::fwrite(J.data(), 1, J.size(), F);
			std::fclose(F);
		}
	}
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

void Screens()
{
	QuietHooks H;
	ss::Session S(H, "ui-shots");
	ss::ui::RiverLine RL(S);
	Emit("boot", RL, 1.0);

	S.CurrentScreen = ss::Screen::Lobby;
	RL.UI.Ptr.Active = true;
	RL.UI.Ptr.X = 520.0f;
	RL.UI.Ptr.Y = 290.0f; // hovering the second row
	Emit("lobby", RL, 2.0);
	S.ConfirmRegister = true;
	RL.UI.Ptr.X = 1200.0f;
	RL.UI.Ptr.Y = 680.0f;
	Emit("lobby_confirm", RL, 2.5);
	S.ConfirmRegister = false;

	// Table: play Full pace until the hero faces a decision after the flop.
	S.Register(0);
	S.CurrentPace = ss::Pace::Full;
	double Now = 0.0;
	int Guard = 0;
	bool Shot = false;
	while (!Shot && ++Guard < 400000 && S.CurrentScreen == ss::Screen::Table)
	{
		Now = Step(S, Now, 1.0 / 30.0);
		if (S.HasPrompt)
		{
			if (S.Board.size() >= 3 && Now - S.Prompt.OpenedAt > 1.2)
			{
				RL.UI.Ptr.X = 980.0f;
				RL.UI.Ptr.Y = 915.0f; // hovering Raise
				Emit("table_decision", RL, Now);
				Shot = true;
			}
			else if (S.Board.size() < 3 && Now - S.Prompt.OpenedAt > 0.3)
			{
				S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
			}
		}
	}
	Expect(Shot, "reached a postflop decision");
	// Carry on to a showdown with cards revealed.
	bool Showdown = false;
	Guard = 0;
	while (!Showdown && ++Guard < 400000 && S.CurrentScreen == ss::Screen::Table)
	{
		Now = Step(S, Now, 1.0 / 30.0);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
		}
		if (S.CurHand && S.CurHand->bComplete)
		{
			for (const ss::SeatVis& V : S.Seats)
			{
				Showdown = Showdown || (V.Present && !V.HandLabel.empty() && V.Winner);
			}
		}
	}
	Expect(Showdown, "reached a showdown");
	RL.UI.Ptr.Active = false;
	Emit("table_showdown", RL, Now + 0.5);
	S.Tab = ss::RightTab::Info;
	S.CurrentBanner.Active = true;
	S.CurrentBanner.Title = "FINAL TABLE";
	S.CurrentBanner.Sub = "9th pays $1.57 \xC2\xB7 1st pays $17.80";
	S.CurrentBanner.At = Now;
	S.CurrentBanner.Color = 0xf2c14e;
	Emit("table_banner", RL, Now + 1.0);
	S.CurrentBanner.Active = false;
	S.Tab = ss::RightTab::Payouts;
	S.Sprinting = true;
	Emit("table_sprint", RL, Now + 1.5);
	S.Sprinting = false;
	S.Tab = ss::RightTab::Chat;
}

void Results()
{
	QuietHooks H;
	ss::Session S(H, "ui-results");
	ss::ui::RiverLine RL(S);
	S.CurrentScreen = ss::Screen::Lobby;
	S.Register(1);
	double Now = 0.0;
	int Guard = 0;
	while (S.CurrentScreen == ss::Screen::Table && ++Guard < 4000000)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
			size_t Best = 0;
			for (size_t I = 1; I < Opts.size(); ++I)
			{
				Best = Opts[I].Ev > Opts[Best].Ev ? I : Best;
			}
			// Mix in some mistakes so the hand review has content.
			const bool Mistake = S.Grades.size() % 5 == 3 && Opts.size() > 1;
			S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Mistake ? (Best + 1) % Opts.size() : Best].Action);
		}
	}
	Expect(S.CurrentScreen == ss::Screen::Results, "reached results");
	Emit("results", RL, Now + 3.0);

	ss::ui::PhoneScreen Phone;
	for (const std::pair<std::string, std::string>& T : H.Texts)
	{
		Phone.Notify(T.first, T.second, Now);
	}
	Phone.Notify("Dee", "Heard about the notice on your door. Tuesday game at the laundromat is still on if you need it. Don't play scared.", Now);
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, ss::ui::PhoneScreen::Width, ss::ui::PhoneScreen::Height, 1.0f);
	Phone.Draw(C, 127.0);
	Expect(Phone.Brightness(Now + 1.0) > 0.9 && Phone.Brightness(Now + 20.0) == 0.0, "phone lights up then sleeps");
	std::printf("  %-14s %6zu vertices\n", "phone", L.Vertices.size());
	if (!OutDir.empty())
	{
		if (FILE* F = std::fopen((OutDir + "/phone.json").c_str(), "wb"))
		{
			const std::string J = L.ToJson();
			std::fwrite(J.data(), 1, J.size(), F);
			std::fclose(F);
		}
	}
}

void Clicks()
{
	// A click on "Log in" moves the boot screen to the lobby; clicks elsewhere do nothing.
	QuietHooks H;
	ss::Session S(H, "ui-clicks");
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	auto Frame = [&](float X, float Y, bool Down, bool Pressed, bool Released) {
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		RL.Draw(C, 1.0);
	};
	Frame(100.0f, 900.0f, true, true, false);
	Frame(100.0f, 900.0f, false, false, true);
	Expect(S.CurrentScreen == ss::Screen::Boot, "click outside does nothing");
	Frame(800.0f, 550.0f, true, true, false);
	Frame(800.0f, 550.0f, false, false, true);
	Expect(S.CurrentScreen == ss::Screen::Lobby, "log in button works");
	Expect(H.Texts.size() == 1, "logging in triggers Dee's text");
	// Select the hyper sprint row, register, confirm.
	Frame(300.0f, 290.0f, true, true, false);
	Frame(300.0f, 290.0f, false, false, true);
	Expect(S.Selected == 1, "row click selects the event");
	Frame(1300.0f, 700.0f, true, true, false);
	Frame(1300.0f, 700.0f, false, false, true);
	Expect(S.ConfirmRegister, "register asks for confirmation");
	Frame(1150.0f, 700.0f, true, true, false);
	Frame(1150.0f, 700.0f, false, false, true);
	Expect(S.CurrentScreen == ss::Screen::Table && S.T != nullptr, "confirm registers and opens the table");
}
} // namespace ui_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	ui_test::Clicks();
	ui_test::Screens();
	ui_test::Results();
	if (ui_test::Failures == 0)
	{
		std::printf("ui tests: all passed\n");
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
