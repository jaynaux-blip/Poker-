// Renders the RiverLine client and the phone screen into draw lists for a set
// of game states and writes them as JSON. web/scripts/render-drawlists.mjs
// replays them in Chromium so the C++ UI can be compared with the prototype.
// Usage: ui_test <out-dir>   (with no argument it only checks the frames draw)
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/PropArt.h"
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
	RL.UI.Ptr.Y = 490.0f; // hovering a schedule row
	Emit("lobby", RL, 2.0);
	S.ConfirmRegister = true;
	RL.UI.Ptr.X = 1200.0f;
	RL.UI.Ptr.Y = 890.0f;
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

void Props()
{
	struct Item
	{
		const char* Name;
		float W;
		float H;
	};
	const Item Items[7] = {{"prop_notice", ss::ui::props::NoticeW, ss::ui::props::NoticeH}, {"prop_bill", ss::ui::props::BillW, ss::ui::props::BillH}, {"prop_note", ss::ui::props::NoteSize, ss::ui::props::NoteSize},
		{"prop_poster", ss::ui::props::PosterW, ss::ui::props::PosterH}, {"prop_neon", ss::ui::props::NeonW, ss::ui::props::NeonH}, {"prop_keyboard", ss::ui::props::KeyboardW, ss::ui::props::KeyboardH}, {"prop_note2", ss::ui::props::NoteSize, ss::ui::props::NoteSize}};
	TableMeasurer M;
	for (int I = 0; I < 7; ++I)
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, Items[I].W, Items[I].H, 1.0f);
		switch (I)
		{
		case 0: ss::ui::props::EvictionNotice(C); break;
		case 1: ss::ui::props::PowerBill(C); break;
		case 2: ss::ui::props::StickyNote(C, {"BR: $2.37", "DON'T", "TILT."}, ss::ui::Hex(0xf6e27a)); break;
		case 3: ss::ui::props::Poster(C); break;
		case 4: ss::ui::props::NeonSign(C); break;
		case 5: ss::ui::props::Keyboard(C); break;
		default: ss::ui::props::StickyNote(C, {"RENT", "FRIDAY", "$1,225"}, ss::ui::Hex(0xff9cb8)); break;
		}
		Expect(!L.Cmds.empty(), "prop drew something");
		if (!OutDir.empty())
		{
			if (FILE* F = std::fopen((OutDir + "/" + Items[I].Name + ".json").c_str(), "wb"))
			{
				const std::string J = L.ToJson();
				std::fwrite(J.data(), 1, J.size(), F);
				std::fclose(F);
			}
		}
	}
}

// ------------------------------------------------------------------ front end

struct MenuHooks : ss::ui::FrontEndHooks
{
	int Continues = 0;
	int Resumes = 0;
	int Quits = 0;
	int QuitsToMenu = 0;
	int SettingsChanges = 0;
	int Sounds = 0;
	std::string NewGameName;
	void UiSound(ss::SoundId, double) override { ++Sounds; }
	void Continue() override { ++Continues; }
	void NewGame(const std::string& Name) override { NewGameName = Name; }
	void Resume() override { ++Resumes; }
	void QuitToMenu() override { ++QuitsToMenu; }
	void QuitGame() override { ++Quits; }
	void SettingsChanged(const ss::ui::GameSettings&) override { ++SettingsChanges; }
};

void EmitMenu(const std::string& Name, ss::ui::FrontEnd& Fe, double Now, float Width = 1920.0f)
{
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, Width, ss::ui::FrontEnd::Height, 1.0f);
	Fe.Draw(C, Now);
	Expect(!L.Cmds.empty(), "menu frame drew something");
	std::printf("  %-22s %6zu vertices %4zu commands\n", Name.c_str(), L.Vertices.size(), L.Cmds.size());
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

/** Draws a few frames so entrance animations settle. */
double Settle(ss::ui::FrontEnd& Fe, double Now, double Seconds)
{
	TableMeasurer M;
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1920.0f, ss::ui::FrontEnd::Height, 1.0f);
		Now += 1.0 / 30.0;
		Fe.Draw(C, Now);
	}
	return Now;
}

void FrontEndFlows()
{
	MenuHooks H;
	ss::ui::FrontEnd Fe(H);
	double Now = 0.0;
	Fe.Open(ss::ui::FrontEnd::Page::Attract, Now);
	Now = Settle(Fe, Now, 1.0);
	Fe.Char('x', Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::Main, "any key leaves the title screen");
	Now = Settle(Fe, Now, 1.0);
	Fe.Key("Enter", Now); // no save: New Game is selected
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::NewGame, "Enter on New Game opens it");
	for (int I = 0; I < 20; ++I)
	{
		Fe.Key("Backspace", Now);
	}
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::NewGame && H.NewGameName.empty(), "an empty name is refused");
	for (const char Ch : std::string("ace high!"))
	{
		Fe.Char(static_cast<uint32_t>(static_cast<unsigned char>(Ch)), Now);
	}
	Expect(Fe.ScreenName == "acehigh", "only valid name characters are typed");
	Fe.Key("Enter", Now);
	Expect(H.NewGameName == "acehigh" && !Fe.IsOpen(), "Enter begins a new game and closes the menu");

	// Pause menu: settings round trip, then resume.
	Fe.Open(ss::ui::FrontEnd::Page::Pause, Now);
	Expect(Fe.IsPaused() && !Fe.WantsEstablishingShot(), "the pause menu pauses without the establishing shot");
	Now = Settle(Fe, Now, 0.5);
	Expect(Fe.Backdrop(Now) > 0.99f, "the pause menu blurs the scene");
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::Settings, "pause opens settings");
	Fe.Key("Left", Now); // quality: Epic -> High
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now); // ray tracing off
	Expect(Fe.Settings.Quality == 2 && !Fe.Settings.RayTracing && H.SettingsChanges == 2, "settings change and report");
	Fe.Key("E", Now);
	Fe.Key("E", Now);
	Fe.Key("Left", Now); // master volume 80 -> 75
	Expect(Fe.Settings.MasterVolume == 75, "audio tab slider steps by 5");
	ss::ui::GameSettings Back;
	Expect(ss::ui::GameSettings::Parse(Fe.Settings.Serialize(), Back) && Back.MasterVolume == 75 && Back.Quality == 2 && !Back.RayTracing, "settings survive a save round trip");
	Fe.Key("Escape", Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::Pause, "Escape returns to the pause menu");
	Fe.Key("Escape", Now);
	Expect(H.Resumes == 1 && !Fe.IsOpen(), "Escape on the pause menu resumes");

	// Quit asks first.
	Fe.Open(ss::ui::FrontEnd::Page::Main, Now);
	Fe.Key("Escape", Now);
	Fe.Key("Enter", Now); // Cancel is the default
	Expect(H.Quits == 0 && Fe.Current() == ss::ui::FrontEnd::Page::Main, "quit defaults to cancel");
	Fe.Key("Escape", Now);
	Fe.Key("Left", Now);
	Fe.Key("Enter", Now);
	Expect(H.Quits == 1, "confirming quits");
	Expect(H.Sounds > 10, "the menus make sounds");
}

void FrontEndScreens()
{
	MenuHooks H;
	ss::ui::FrontEnd Fe(H);
	Fe.Info.HasSave = true;
	Fe.Info.HeroName = "grinder_3c";
	Fe.Info.BankrollCents = 1864;
	Fe.Info.Tournaments = 3;
	Fe.Info.BestFinish = "12th of 180";
	Fe.Info.LastResult = "Night Owl Turbo \xC2\xB7 45th";
	Fe.Info.Resolutions = {"2560 x 1440", "1920 x 1080", "1600 x 900"};
	Fe.Info.Status = {{"BANKROLL", "$18.64"}, {"TIME", "3:42 AM"}, {"EVENT", "Night Owl Turbo"}, {"PLAYERS LEFT", "212"}, {"YOUR RANK", "38th"}, {"STACK", "24,150"}};
	Fe.Info.InTournament = true;
	double Now = 100.0;
	Fe.Open(ss::ui::FrontEnd::Page::Attract, Now);
	EmitMenu("menu_attract", Fe, Now + 3.0);
	Now += 3.0;
	Fe.Key("Any", Now);
	Now = Settle(Fe, Now, 1.2);
	EmitMenu("menu_main", Fe, Now);
	Fe.Key("Down", Now);
	Now = Settle(Fe, Now, 0.6);
	EmitMenu("menu_main_newgame", Fe, Now);
	EmitMenu("menu_main_ultrawide", Fe, Now + 0.05, 2520.0f);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 1.0);
	EmitMenu("menu_newgame", Fe, Now);
	Fe.Key("Escape", Now);
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 1.0);
	Fe.Key("Down", Now);
	Now = Settle(Fe, Now, 0.5);
	EmitMenu("menu_settings_graphics", Fe, Now);
	Fe.Key("E", Now);
	Fe.Key("E", Now);
	Now = Settle(Fe, Now, 0.6);
	EmitMenu("menu_settings_audio", Fe, Now);
	Fe.Key("Escape", Now);
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 6.0);
	EmitMenu("menu_credits", Fe, Now);
	Fe.Close(Now);
	Fe.Open(ss::ui::FrontEnd::Page::Pause, Now);
	Now = Settle(Fe, Now, 1.0);
	EmitMenu("menu_pause", Fe, Now);
	Fe.Key("Down", Now);
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 0.5);
	EmitMenu("menu_pause_confirm", Fe, Now);
	Fe.Gamepad = true;
	Fe.Key("Escape", Now);
	Fe.Close(Now);
	Fe.Open(ss::ui::FrontEnd::Page::Attract, Now);
	EmitMenu("menu_attract_gamepad", Fe, Now + 3.0);
}

/** Draws frames (and runs the session) for a while, so animations and the lobby clock move on. */
double Run(ss::Session& S, ss::ui::RiverLine& RL, double Now, double Seconds)
{
	TableMeasurer M;
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
	}
	return Now;
}

void NetScreens()
{
	using Page = ss::ui::RiverLine::Page;
	QuietHooks H;
	ss::Session S(H, "ui-net");
	ss::ui::RiverLine RL(S);
	S.CurrentScreen = ss::Screen::Lobby;
	RL.UI.Ptr.Active = true;
	RL.UI.Ptr.X = 640.0f;
	RL.UI.Ptr.Y = 560.0f; // hovering a schedule row
	double Now = Run(S, RL, 10.0, 1.5);
	Emit("net_lobby", RL, Now);
	RL.UI.Ptr.X = 400.0f;
	RL.UI.Ptr.Y = 200.0f; // over the banner (it stops rotating)
	Now = Run(S, RL, Now, 8.0);
	Emit("net_lobby_slide", RL, Now);
	RL.SetFilter(ss::ui::RiverLine::Filter::Playable, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("net_lobby_playable", RL, Now);
	for (Page P : {Page::Series, Page::Leaderboards, Page::News, Page::Career})
	{
		RL.OpenPage(P, Now);
		Now = Run(S, RL, Now, 1.5);
		const char* Names[5] = {"net_lobby", "net_series", "net_boards", "net_news", "net_career"};
		Emit(Names[static_cast<int>(P)], RL, Now);
	}
	RL.ShowSeries("rcop", Now);
	RL.OpenPage(Page::Series, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("net_series_rcop", RL, Now);

	// Later that night: a Micro Madness title and a Night Owl cash.
	ss::HistoryEntry Owl;
	Owl.Name = "$1.10 Night Owl Turbo";
	Owl.Place = 31;
	Owl.Entrants = 1000;
	Owl.Prize = 380;
	Owl.AccuracyPct = 82.0;
	Owl.BuyInCents = 110;
	Owl.EventId = "night-owl@1560";
	ss::HistoryEntry Title;
	Title.Name = "MM #26: Night Crawler";
	Title.Place = 1;
	Title.Entrants = 1184;
	Title.Prize = 45000;
	Title.AccuracyPct = 91.0;
	Title.BuyInCents = 220;
	Title.EventId = "mm-26@1530";
	S.History = {Title, Owl};
	S.BankrollCents += 45380;
	S.LobbyMinutes = 5.0 * 60.0 + 12.0;
	for (Page P : {Page::Leaderboards, Page::News, Page::Career})
	{
		RL.OpenPage(P, Now);
		Now = Run(S, RL, Now, 1.5);
		const char* Names[5] = {"", "", "net_boards_after", "net_news_after", "net_career_after"};
		Emit(Names[static_cast<int>(P)], RL, Now);
	}
	RL.ShowBoard(ss::net::Board::Series, Now);
	RL.OpenPage(Page::Leaderboards, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("net_boards_series", RL, Now);
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
	// Show what can be played now, pick the hyper sprint, register, confirm.
	Frame(130.0f, 330.0f, true, true, false);
	Frame(130.0f, 330.0f, false, false, true);
	Frame(130.0f, 330.0f, false, false, false);
	Expect(RL.ListedEvents().size() >= 4, "the Playable chip filters the schedule");
	int Row = -1;
	for (size_t I = 0; I < RL.ListedEvents().size() && I < 9; ++I)
	{
		Row = Row < 0 && RL.ListedEvents()[I].rfind("hyper-sprint@", 0) == 0 ? static_cast<int>(I) : Row;
	}
	Expect(Row >= 0, "a hyper sprint is open");
	const float RowY = 404.0f + static_cast<float>(Row) * 60.0f + 27.0f;
	Frame(300.0f, RowY, true, true, false);
	Frame(300.0f, RowY, false, false, true);
	Expect(Row >= 0 && RL.SelectedEvent() == RL.ListedEvents()[static_cast<size_t>(Row)], "row click selects the event");
	Frame(1300.0f, 890.0f, true, true, false);
	Frame(1300.0f, 890.0f, false, false, true);
	Expect(S.ConfirmRegister, "register asks for confirmation");
	Frame(1200.0f, 890.0f, true, true, false);
	Frame(1200.0f, 890.0f, false, false, true);
	Expect(S.CurrentScreen == ss::Screen::Table && S.T != nullptr && S.T->Spec.Id.rfind("hyper-sprint@", 0) == 0, "confirm registers and opens the table");
	// Navigation.
	S.CurrentScreen = ss::Screen::Lobby;
	S.T.reset();
	Frame(500.0f, 34.0f, true, true, false);
	Frame(500.0f, 34.0f, false, false, true);
	Expect(RL.CurrentPage() == ss::ui::RiverLine::Page::Leaderboards, "the top bar opens the leaderboards");
}
} // namespace ui_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	ui_test::NetScreens();
	ui_test::Clicks();
	ui_test::Screens();
	ui_test::Results();
	ui_test::Props();
	ui_test::FrontEndFlows();
	ui_test::FrontEndScreens();
	if (ui_test::Failures == 0)
	{
		std::printf("ui tests: all passed\n");
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
