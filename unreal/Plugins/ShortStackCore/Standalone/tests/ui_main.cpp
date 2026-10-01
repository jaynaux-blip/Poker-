// Renders the RiverLine client and the phone screen into draw lists for a set
// of game states and writes them as JSON. web/scripts/render-drawlists.mjs
// replays them in Chromium so the C++ UI can be compared with the prototype.
// Usage: ui_test <out-dir>   (with no argument it only checks the frames draw)
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/PropArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "TestFontMetrics.h"

#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

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

/** Profile pictures: names pick fitting icons, everyone gets one, and a gallery of the set for review. */
void Avatars()
{
	using ss::ui::AvatarFor;
	using ss::ui::AvatarIcon;
	Expect(AvatarFor("ElTiburon").Icon == AvatarIcon::Shark && AvatarFor("C0ldSh4rk").Icon == AvatarIcon::Shark, "shark names get sharks, leetspeak too");
	Expect(AvatarFor("CoolerKing").Icon == AvatarIcon::Crown && AvatarFor("ReiDoRio").Icon == AvatarIcon::Crown, "kings get crowns");
	Expect(AvatarFor("lazy_owl").Icon == AvatarIcon::Owl && AvatarFor("CoffeeAndCards").Icon == AvatarIcon::Coffee, "owls and coffee");
	Expect(AvatarFor("Volkov").Icon != AvatarIcon::Wolf && AvatarFor("Volk88").Icon == AvatarIcon::Wolf, "short words match whole words only");
	Expect(AvatarFor(ss::RivalName).Icon == AvatarIcon::Ghost && AvatarFor(ss::RivalName).Frame == ss::ui::AvatarFrame::Neon, "the rival is the ghost");
	Expect(AvatarFor("PocketRockets").Icon == AvatarFor("PocketRockets").Icon && AvatarFor("PocketRockets").Bg == AvatarFor("PocketRockets").Bg, "avatars are stable");
	std::map<int, int> Seen;
	bool GhostTaken = false;
	for (const ss::net::Player& P : ss::net::Shared().Players())
	{
		const ss::ui::AvatarSpec A = AvatarFor(P.Name);
		++Seen[static_cast<int>(A.Icon)];
		GhostTaken = GhostTaken || (A.Icon == AvatarIcon::Ghost && !P.Rival);
	}
	Expect(!GhostTaken, "nobody but the rival wears the ghost");
	Expect(Seen.size() >= 30, "the network uses most of the set");
	Expect(Seen[static_cast<int>(AvatarIcon::Initials)] < static_cast<int>(ss::net::Shared().Players().size() / 3), "initials are the exception");
	std::printf("  avatars        %zu icons across %zu regulars\n", Seen.size(), ss::net::Shared().Players().size());

	TableMeasurer M;
	ss::ui::DrawList L;
	const float W = 1600.0f;
	const float H = 1000.0f;
	ss::ui::Canvas C(L, M, W, H, 1.0f);
	C.FillRect({0.0f, 0.0f, W, H}, ss::ui::Paint::Linear({0.0f, 0.0f}, {0.0f, H}, ss::ui::Hex(0x111a2b), ss::ui::Hex(0x0a0f1a)));
	C.Text("RIVERLINE AVATARS", 48.0f, 62.0f, ss::ui::Ts(30.0f, 900, ss::ui::Hex(0xffffff)));
	C.Text("Every icon, then the frames, then regulars as the lobby shows them", 48.0f, 92.0f, ss::ui::Ts(16.0f, 500, ss::ui::Hex(0x8b9bb4)));
	const int Icons = static_cast<int>(AvatarIcon::Count);
	for (int I = 0; I < Icons; ++I)
	{
		const float X = 92.0f + static_cast<float>(I % 12) * 120.0f;
		const float Y = 170.0f + static_cast<float>(I / 12) * 128.0f;
		ss::ui::AvatarSpec A = AvatarFor("gallery" + std::to_string(I * 7));
		A.Icon = static_cast<AvatarIcon>(I);
		A.Frame = ss::ui::AvatarFrame::None;
		A.Initials = "JK";
		if (A.Icon == AvatarIcon::Ghost)
		{
			A = AvatarFor(ss::RivalName);
		}
		ss::ui::DrawAvatar(C, X, Y, 38.0f, A);
		C.Text(ss::ui::AvatarIconName(A.Icon), X, Y + 62.0f, ss::ui::Ts(13.0f, 600, ss::ui::Hex(0xc7d2e3), ss::ui::Align::Center));
	}
	const char* Frames[5] = {"None", "Ring", "Chip", "Gold (Team RiverLine)", "Neon (you, the rival)"};
	for (int F = 0; F < 5; ++F)
	{
		const float X = 120.0f + static_cast<float>(F) * 230.0f;
		ss::ui::AvatarSpec A = AvatarFor("SetMiner");
		A.Frame = static_cast<ss::ui::AvatarFrame>(F);
		A.Rim = F == 1 ? 0xf472b6 : 0x27d3c3;
		ss::ui::DrawAvatar(C, X, 590.0f, 42.0f, A);
		C.Text(Frames[F], X, 666.0f, ss::ui::Ts(13.0f, 600, ss::ui::Hex(0xc7d2e3), ss::ui::Align::Center));
	}
	const std::vector<ss::net::Player>& People = ss::net::Shared().Players();
	for (int I = 0; I < 30; ++I)
	{
		const ss::net::Player& P = People[static_cast<size_t>(I * 13 % static_cast<int>(People.size()))];
		const float X = 60.0f + static_cast<float>(I % 6) * 256.0f;
		const float Y = 730.0f + static_cast<float>(I / 6) * 52.0f;
		ss::ui::AvatarSpec A = AvatarFor(P.Name);
		if (P.Pro)
		{
			A.Frame = ss::ui::AvatarFrame::Gold;
		}
		ss::ui::DrawAvatar(C, X, Y, 18.0f, A);
		C.Text(P.Name, X + 28.0f, Y + 1.0f, ss::ui::Ts(15.0f, 700, ss::ui::Hex(0xe6edf7), ss::ui::Align::Left, ss::ui::Baseline::Middle));
		C.Text(P.Country, X + 28.0f, Y + 18.0f, ss::ui::Ts(11.0f, 600, ss::ui::Hex(0x6b7a92), ss::ui::Align::Left, ss::ui::Baseline::Middle, true));
	}
	if (!OutDir.empty())
	{
		if (FILE* F = std::fopen((OutDir + "/avatars.json").c_str(), "wb"))
		{
			const std::string J = L.ToJson();
			std::fwrite(J.data(), 1, J.size(), F);
			std::fclose(F);
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

/** The laptop apps, the time-lapse, a bounty table and the new result screens. */
void AppScreens()
{
	using App = ss::ui::RiverLine::App;
	QuietHooks H;
	ss::Session S(H, "ui-apps");
	ss::ui::RiverLine RL(S);
	S.CurrentScreen = ss::Screen::Lobby;
	RL.UI.Ptr.Active = true;
	RL.UI.Ptr.X = 300.0f;
	RL.UI.Ptr.Y = 880.0f;
	double Now = Run(S, RL, 10.0, 1.0);
	Emit("net_lobby_taskbar", RL, Now);
	RL.OpenApp(App::ShiftLink, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("app_shiftlink", RL, Now);
	RL.OpenApp(App::Burner, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("app_burner", RL, Now);
	RL.ShowSleepMenu(true);
	RL.UI.Ptr.X = 520.0f;
	RL.UI.Ptr.Y = 820.0f;
	Now = Run(S, RL, Now, 0.3);
	Emit("app_sleep", RL, Now);
	RL.ShowSleepMenu(false);
	// A night shift at the Quik Stop: the time-lapse, then the result.
	Expect(S.StartActivity("quikstop").empty(), "the night shift starts from the app");
	Now = Run(S, RL, Now, 1.6);
	Emit("app_skip", RL, Now);
	Now = Run(S, RL, Now, 4.5);
	Expect(S.HasOutcome, "the shift's result is up");
	Emit("app_outcome", RL, Now);
	S.HasOutcome = false;
	RL.OpenApp(App::Bank, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("app_bank", RL, Now);
	// Night: Marcus trusts you now, and the cops have noticed.
	S.LobbyMinutes = 1440.0 + 21.0 * 60.0 + 30.0;
	Now = Run(S, RL, Now, 0.2);
	S.Life.Energy = 88.0;
	S.Life.Runs = 2;
	S.Life.Heat = 38.0;
	S.Life.EarnedHustles = 33600;
	S.History.push_back(ss::HistoryEntry());
	S.History.back().Name = "$1.10 Night Owl Turbo";
	S.History.back().Place = 120;
	S.History.back().Entrants = 1000;
	S.History.back().Prize = 165;
	RL.OpenApp(App::Burner, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("app_burner_night", RL, Now);
	RL.ShowContact(1);
	Now = Run(S, RL, Now, 0.5);
	Emit("app_burner_sam", RL, Now);

	// A progressive knockout, mid-hand, just after a knockout.
	{
		QuietHooks Hp;
		ss::Session P(Hp, "ui-pko");
		ss::ui::RiverLine Rp(P);
		P.CurrentScreen = ss::Screen::Lobby;
		P.Life.Unlocks.insert("bounty");
		P.BankrollCents = 2000;
		ss::net::EventInstance Pko;
		Expect(ss::net::Shared().FindInstance("hh-110@1500", Pko), "the 1 AM PKO is scheduled");
		P.RegisterEvent(ss::net::Shared().Listing(Pko, nullptr, P.Unlocks()));
		P.CurrentPace = ss::Pace::Full;
		double T0 = 0.0;
		int Guard = 0;
		while (++Guard < 200000 && !(P.HasPrompt && T0 - P.Prompt.OpenedAt > 1.0 && P.Board.size() >= 3))
		{
			T0 = Step(P, T0, 1.0 / 30.0);
			if (P.HasPrompt && P.Board.size() < 3 && T0 - P.Prompt.OpenedAt > 0.3)
			{
				P.HeroAct(P.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
			}
		}
		P.Knockouts = 2;
		P.BountyWon = 75;
		P.LastBountyAt = T0 - 0.5;
		P.LastBountyCents = 50;
		P.LastBountyName = "NutDoctor";
		Rp.UI.Ptr.Active = false;
		Emit("table_pko", Rp, T0);
		// Results: a seat, then a deep PKO run.
		P.HasResults = true;
		P.CurrentScreen = ss::Screen::Results;
		P.ResultsAt = T0 - 3.0;
		P.LastResults = ss::Results();
		P.LastResults.EventName = "Step 2 \xC2\xB7 RCOP Main";
		P.LastResults.Place = 4;
		P.LastResults.Entrants = 60;
		P.LastResults.Hands = 112;
		P.LastResults.AccuracyPct = 86.0;
		P.LastResults.SeatWon = "step3";
		P.LastResults.SeatValueCents = 5500;
		Emit("results_seat", Rp, T0);
		P.LastResults.EventName = "$1.10 Headhunter PKO";
		P.LastResults.SeatWon.clear();
		P.LastResults.Place = 7;
		P.LastResults.Entrants = 896;
		P.LastResults.PrizeCents = 1240;
		P.LastResults.BountyCents = 375;
		P.LastResults.Knockouts = 6;
		Emit("results_pko", Rp, T0);
	}
}

/** Multi-tabling: the tabs, a table waiting behind another, the tile view and its buttons, the lobby while seated, a closed table, the sitting's results. */
std::vector<ss::LobbyEvent> OpenEvents(ss::Session& S, size_t Count, int MaxEntrants)
{
	const ss::net::Network& Net = ss::net::Shared();
	const double World = S.WorldMinutes();
	std::vector<ss::LobbyEvent> Out;
	std::set<std::string> Names;
	for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 60.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents <= 1100 && L.Spec.Entrants <= MaxEntrants && Out.size() < Count && Names.insert(L.Spec.Name).second)
		{
			Out.push_back(L);
		}
	}
	return Out;
}

void MultiScreens()
{
	QuietHooks H;
	ss::Session S(H, "ui-multi");
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 6000;
	auto Frame = [&](float X, float Y, bool Down, bool Pressed, bool Released, double At) {
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		RL.Draw(C, At);
	};
	const std::vector<ss::LobbyEvent> Picks = OpenEvents(S, 4, 3000);
	Expect(Picks.size() == 4, "four events open for the multi-table screens");
	if (Picks.size() < 4)
	{
		return;
	}
	double Now = 1.0;
	for (const ss::LobbyEvent& L : Picks)
	{
		S.RegisterEvent(L);
		Now = Run(S, RL, Now, 3.0);
	}
	Expect(S.TableCount() == 4, "four tables open");
	RL.UI.Ptr.Active = false;

	// One table in front, another waiting behind it: its tab flashes.
	S.AutoFocus = false;
	for (int Guard = 0; Guard < 30 * 600 && !(S.TablesWaiting() >= 1 && !S.HasPrompt); ++Guard)
	{
		Now = Run(S, RL, Now, 1.0 / 30.0);
	}
	Expect(S.TablesWaiting() >= 1 && !S.HasPrompt, "a table behind the one in front waits for the player");
	Now = Run(S, RL, Now, 0.4);
	Emit("multi_tabs", RL, Now);

	// Every table at once, each with its own buttons.
	S.Tiled = true;
	S.AutoFocus = true;
	for (int Guard = 0; Guard < 30 * 600 && S.TablesWaiting() < 1; ++Guard)
	{
		Now = Run(S, RL, Now, 1.0 / 30.0);
	}
	Now = Run(S, RL, Now, 0.5);
	Emit("multi_tiles", RL, Now);
	int Waiting = -1;
	for (int I = 0; I < S.TableCount(); ++I)
	{
		Waiting = Waiting < 0 && S.Glance(I).YourTurn ? I : Waiting;
	}
	Expect(Waiting >= 0, "a tile waits for the player");
	if (Waiting >= 0)
	{
		// Fold from that tile's own button.
		const float Tx = 8.0f + static_cast<float>(Waiting % 2) * 796.0f;
		const float Ty = 72.0f + static_cast<float>(Waiting / 2) * 445.0f;
		Frame(Tx + 52.0f, Ty + 437.0f - 30.0f, true, true, false, Now);
		Frame(Tx + 52.0f, Ty + 437.0f - 30.0f, false, false, true, Now);
		Expect(!S.Glance(Waiting).YourTurn && S.FocusedTable() == Waiting, "a tile's button acts at its table and brings it forward");
		RL.UI.Ptr.Active = false;
	}

	// Back in the lobby with the tables still running: the tabs come along, and a seated event opens its table.
	S.ShowLobby();
	RL.OpenPage(ss::ui::RiverLine::Page::Lobby, Now);
	RL.SelectEvent(Picks[1].Spec.Id);
	Now = Run(S, RL, Now, 1.2);
	Emit("multi_lobby", RL, Now);
	Frame(1300.0f, 890.0f, true, true, false, Now);
	Frame(1300.0f, 890.0f, false, false, true, Now);
	Expect(S.CurrentScreen == ss::Screen::Table && S.Glance(S.FocusedTable()).EventId == Picks[1].Spec.Id, "Open table goes to the event's table");
	RL.UI.Ptr.Active = false;

	// A table that closed while the others play on.
	S.Tiled = false;
	ss::FinishedTable Done;
	Done.Result.EventName = "$3.30 Daily Grind";
	Done.Result.Place = 41;
	Done.Result.Entrants = 501;
	Done.Result.PrizeCents = 612;
	Done.Result.AccuracyPct = 84.2;
	Done.Result.Grades.resize(9);
	Done.At = Now - 1.0;
	S.Finished.push_back(Done);
	Now = Run(S, RL, Now, 0.6);
	Emit("multi_toast", RL, Now);

	// Two tables tiled, played to the end: the results add up the sitting.
	ss::Session Two(H, "ui-multi-two");
	ss::ui::RiverLine RL2(Two);
	Two.CurrentScreen = ss::Screen::Lobby;
	Two.BankrollCents = 3000;
	const std::vector<ss::LobbyEvent> Small = OpenEvents(Two, 2, 1500);
	Expect(Small.size() == 2, "two small events open");
	if (Small.size() < 2)
	{
		return;
	}
	Two.RegisterEvent(Small[0]);
	Two.RegisterEvent(Small[1]);
	Two.Tiled = true;
	double At = 1.0;
	for (int Guard = 0; Guard < 30 * 600 && Two.TablesWaiting() < 1; ++Guard)
	{
		At = Run(Two, RL2, At, 1.0 / 30.0);
	}
	At = Run(Two, RL2, At, 0.5);
	Emit("multi_tiles2", RL2, At);
	ss::Rng Choice("ui-multi-two");
	for (int Guard = 0; Guard < 30 * 6000 && Two.CurrentScreen == ss::Screen::Table; ++Guard)
	{
		At += 1.0 / 30.0;
		Two.Update(At);
		for (int I = 0; I < Two.TableCount(); ++I)
		{
			Two.WithTable(I, [&]() { Two.CurrentPace = Two.CurrentPace == ss::Pace::Full ? ss::Pace::Full : ss::Pace::Sprint; });
			if (Two.Glance(I).YourTurn)
			{
				Two.HeroActAt(I, Choice.Chance(0.5) ? ss::PlayerAction::Call() : ss::PlayerAction::Fold());
				break;
			}
		}
	}
	Expect(Two.CurrentScreen == ss::Screen::Results && Two.LastResults.SessionEvents == 2, "the last of two tables brings the sitting's results");
	At = Run(Two, RL2, At, 1.6);
	Emit("results_sitting", RL2, At);
	std::printf("  multi          4 tables (tabs, tiles, lobby, toast), 2 tiled to the end: net %s\n", ss::Money(Two.LastResults.SessionNetCents).c_str());
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
	// The taskbar opens ShiftLink, and a night shift starts from its card.
	Frame(170.0f, 981.0f, true, true, false);
	Frame(170.0f, 981.0f, false, false, true);
	Expect(RL.CurrentApp() == ss::ui::RiverLine::App::ShiftLink, "the taskbar opens ShiftLink");
	Frame(290.0f, 516.0f, true, true, false);
	Frame(290.0f, 516.0f, false, false, true);
	Expect(S.TimeSkip.Active && S.TimeSkip.Result.ActivityId == "quikstop", "Take shift starts the Quik Stop shift");
}
} // namespace ui_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	ui_test::NetScreens();
	ui_test::AppScreens();
	ui_test::Clicks();
	ui_test::Screens();
	ui_test::Results();
	ui_test::Props();
	ui_test::Avatars();
	ui_test::MultiScreens();
	ui_test::FrontEndFlows();
	ui_test::FrontEndScreens();
	if (ui_test::Failures == 0)
	{
		std::printf("ui tests: all passed\n");
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
