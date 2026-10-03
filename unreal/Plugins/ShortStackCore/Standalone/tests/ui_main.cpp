// Renders the RiverLine client and the phone screen into draw lists for a set
// of game states and writes them as JSON. web/scripts/render-drawlists.mjs
// replays them in Chromium so the C++ UI can be compared with the prototype.
// Usage: ui_test <out-dir>   (with no argument it only checks the frames draw)
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/PropArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/StreamArt.h"
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

// ------------------------------------------------------------------ GearDrop and Kast art

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

/** Every product picture, every emote, the facecam in each mood and gear level, and the desk filling up. */
void StreamGallery()
{
	namespace sa = ss::ui::streamart;
	TableMeasurer M;
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		C.FillRect({0.0f, 0.0f, 1600.0f, 1000.0f}, ss::ui::Hex(0xf4f5f7));
		const std::vector<ss::gear::Item>& Items = ss::gear::Catalog();
		for (size_t I = 0; I < Items.size(); ++I)
		{
			const float X = 20.0f + static_cast<float>(I % 7) * 225.0f;
			const float Y = 20.0f + static_cast<float>(I / 7) * 240.0f;
			C.FillRoundRect({X, Y, 210.0f, 225.0f}, 14.0f, ss::ui::Hex(0xffffff));
			C.FillRoundRect({X + 8.0f, Y + 8.0f, 194.0f, 170.0f}, 10.0f, ss::ui::Paint::Linear({X, Y}, {X, Y + 170.0f}, ss::ui::Mix(ss::ui::Hex(Items[I].Color), ss::ui::Hex(0xffffff), 0.82f), ss::ui::Mix(ss::ui::Hex(Items[I].Color), ss::ui::Hex(0xffffff), 0.6f)));
			sa::Product(C, Items[I].Pic, {X + 8.0f, Y + 8.0f, 194.0f, 170.0f}, Items[I].Color, 1.0);
			C.Text(Items[I].Name, X + 12.0f, Y + 202.0f, ss::ui::Ts(14.0f, 700, ss::ui::Hex(0x111827), ss::ui::Align::Left, ss::ui::Baseline::Alphabetic, false, 186.0f));
		}
		Expect(L.Vertices.size() > 5000, "every product has a picture");
		WriteList("stream_products", L);
	}
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		C.FillRect({0.0f, 0.0f, 1600.0f, 1000.0f}, ss::ui::Hex(0x0f0b17));
		for (int E = 0; E < static_cast<int>(ss::kast::Emote::Count); ++E)
		{
			sa::Emote(C, E, 30.0f + static_cast<float>(E) * 118.0f, 24.0f, 80.0f);
			sa::Emote(C, E, 60.0f + static_cast<float>(E) * 118.0f, 118.0f, 22.0f);
		}
		const ss::kast::Mood Moods[] = {ss::kast::Mood::Focus, ss::kast::Mood::Happy, ss::kast::Mood::Hyped, ss::kast::Mood::Shocked, ss::kast::Mood::Tilted, ss::kast::Mood::Laugh};
		for (int Tier = 0; Tier < 3; ++Tier)
		{
			ss::gear::Owned Own;
			if (Tier >= 1)
			{
				Own = {{"webcam-720", 0.0}, {"mic-usb", 0.0}, {"ring-light", 0.0}};
			}
			if (Tier >= 2)
			{
				Own = {{"mirrorless", 0.0}, {"mic-xlr", 0.0}, {"key-lights", 0.0}, {"headphones", 0.0}};
			}
			for (int K = 0; K < 6; ++K)
			{
				sa::Cam Look;
				Look.Gear = ss::gear::Sum(Own);
				Look.Headphones = Own.count("headphones") > 0;
				Look.Face = Moods[K];
				Look.FaceAge = 0.6;
				Look.Time = 2.0 + K;
				sa::Facecam(C, {20.0f + static_cast<float>(K) * 262.0f, 170.0f + static_cast<float>(Tier) * 160.0f, 250.0f, 141.0f}, Look);
			}
		}
		ss::gear::Owned Desk;
		sa::Desk(C, {20.0f, 660.0f, 500.0f, 320.0f}, Desk, false, 1.0);
		Desk = {{"monitor-24", 0.0}, {"ram-32", 0.0}, {"webcam-720", 0.0}, {"mic-usb", 0.0}, {"ring-light", 0.0}, {"plant", 0.0}};
		sa::Desk(C, {550.0f, 660.0f, 500.0f, 320.0f}, Desk, true, 1.0);
		for (const ss::gear::Item& I : ss::gear::Catalog())
		{
			Desk[I.Id] = 0.0;
		}
		Desk.erase("ring-light");
		Desk.erase("mic-usb");
		sa::Desk(C, {1080.0f, 660.0f, 500.0f, 320.0f}, Desk, true, 1.0);
		WriteList("stream_art", L);
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
	S.BankrollCents = 6000 + 14900 + 28900;
	Expect(S.Buy("monitor-24").empty() && S.Buy("monitor-27").empty() && S.BankrollCents == 6000, "two monitors for four tables");
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
/** GearDrop and Kast: the store, the locked studio on the laptop, the upgrade that unlocks it, a stream from the lobby to a
 * table, the channel, the directory, the end-of-stream card. */
void StreamScreens()
{
	QuietHooks H;
	ss::Session S(H, "ui-stream");
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 250000;
	double Now = 1.0;
	auto Frame = [&](float X, float Y, bool Down, bool Pressed, bool Released) {
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
	};
	auto Click = [&](float X, float Y) {
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	};
	RL.UI.Ptr.Active = false;
	RL.OpenApp(ss::ui::RiverLine::App::GearDrop, Now);
	Emit("store", RL, Now + 1.0);
	Now += 1.0;

	// Kast on the laptop: locked.
	RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
	Emit("kast_locked", RL, Now + 1.0);
	Now += 1.0;
	Expect(!S.GoLive().empty() && !S.Streaming(), "the laptop alone can't stream");
	Click(524.0f, 547.0f);
	Expect(RL.CurrentApp() == ss::ui::RiverLine::App::GearDrop && RL.OrderShown() == ss::gear::FirstPcUpgrade().Id, "the locked studio leads to the next PC upgrade");
	Emit("store_order", RL, Now + 0.6);
	Now += 0.6;
	Click(887.0f, 654.0f);
	Expect(S.Owns(ss::gear::FirstPcUpgrade().Id) && S.GearFx().CanStream() && RL.OrderShown().empty(), "placing the order unlocks streaming");
	Emit("store_delivered", RL, Now + 0.4);
	for (const char* Id : {"webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24", "headphones", "plant", "chair", "modbot"})
	{
		Expect(S.Buy(Id).empty(), "GearDrop sells the stream kit");
	}
	Expect(S.Buy("webcam-720") == "You have better.", "no side-grades: a 720p webcam after the 1080p one");
	RL.ShowStoreCategory(static_cast<int>(ss::gear::Category::Stream));
	RL.OpenApp(ss::ui::RiverLine::App::GearDrop, Now);
	Emit("store_stream", RL, Now + 1.0);
	Now += 1.0;

	// A channel four months in (a mid rig, four nights a week on a posted schedule), so the pages have something to show.
	ss::kast::Channel& Ch = S.Channel;
	ss::Rng Fake("ui-stream-channel");
	const double World0 = S.WorldMinutes();
	const int Today = ss::net::DayOf(World0);
	Ch.Followers = 1040;
	Ch.Affiliate = true;
	Ch.MinutesLive = 74.0 * 170.0;
	Ch.Streams = 74;
	Ch.Peak = 96;
	Ch.Milestone = 1000;
	Ch.EarnedSubs = 41250;
	Ch.EarnedTips = 18400;
	Ch.EarnedBits = 6300;
	Ch.EarnedAds = 2170;
	Ch.EarnedSponsors = 9800;
	Ch.PaidCents = 64000;
	Ch.ScheduleDays = 1 | 4 | 16 | 64; // Mon Wed Fri Sun
	Ch.ScheduleStart = 21 * 60;
	Ch.Stage = 4;
	Ch.Xp = 9300.0;
	Ch.Streak = 6;
	Ch.Week = static_cast<int>(std::floor(World0 / (7.0 * ss::net::MinutesPerDay)));
	Ch.WeekStreams = 2;
	Ch.WeekMinutes = 330.0;
	Ch.WeekAnswers = 5;
	Ch.WeekRegulars = 1;
	Ch.WeekRewarded = 4;
	Ch.ProcessedDay = Today;
	for (int Back = 27; Back >= 1; --Back)
	{
		const int D = Today - Back;
		if (((Ch.ScheduleDays >> (((D % 7) + 7) % 7)) & 1) && Back != 9)
		{
			Ch.DaysLive.push_back(D);
		}
	}
	for (size_t K = 0; K < Ch.DaysLive.size(); ++K)
	{
		const int I = static_cast<int>(K);
		ss::kast::StreamLog Lg;
		Lg.Start = static_cast<double>(Ch.DaysLive[Ch.DaysLive.size() - 1 - K]) * ss::net::MinutesPerDay + 21.0 * 60.0;
		Lg.Minutes = 170.0 + 10.0 * static_cast<double>(I % 3);
		Lg.Avg = 36 - I / 2;
		Lg.Peak = Lg.Avg * 8 / 5;
		Lg.Follows = 11 - I / 3;
		Lg.Subs = I % 4 == 0 ? 1 : 0;
		Lg.Cents = 2600 - 70 * I;
		Lg.Returning = 24 - I / 3;
		Lg.OnSchedule = true;
		Ch.Log.push_back(Lg);
	}
	const std::vector<std::string> Clips = {"grinder_3c holds the all-in for 41,200", "the cruelest river (grinder_3c)", "grinder_3c makes the final table of the Night Owl", "grinder_3c sends VelvetRiver home"};
	const double Views[4] = {2640.0, 940.0, 610.0, 380.0};
	for (size_t K = 0; K < Clips.size(); ++K)
	{
		ss::kast::Clip Cl;
		Cl.Title = Clips[K];
		Cl.By = ss::handles::Make(Fake, "US");
		Cl.Views = Views[K];
		Cl.Reach = Cl.Views;
		Cl.At = World0 - 3000.0;
		Ch.Clips.push_back(Cl);
	}
	// The community: two friends from night one, a core of regulars, and a long tail of people who came once.
	const std::pair<const char*, double> Friends[2] = {{"dee_spincycle", 0.91}, {"mei_ng", 0.78}};
	for (const auto& F : Friends)
	{
		ss::kast::Member Mb;
		Mb.Name = F.first;
		Mb.Friend = true;
		Mb.Follower = true;
		Mb.Affinity = F.second + 0.04;
		Mb.Loyalty = F.second;
		Mb.Streams = 66;
		Mb.WatchMinutes = 66.0 * 150.0;
		Mb.Messages = 410;
		Mb.FirstSeen = World0 - 120.0 * ss::net::MinutesPerDay;
		Mb.LastSeen = World0 - 2.0 * ss::net::MinutesPerDay;
		Ch.Members.push_back(Mb);
	}
	for (int K = 0; K < 460; ++K)
	{
		ss::kast::Member Mb;
		Mb.Name = ss::handles::Make(Fake, ss::handles::PickCountry(Fake));
		const double U = Fake.Next();
		Mb.Affinity = 0.05 + 0.9 * U * U * U * U;
		const double Share = 0.25 + 0.75 * Fake.Next();
		Mb.Loyalty = Mb.Affinity * Share;
		const double Seen = Fake.Next();
		Mb.Streams = 1 + static_cast<int>(Mb.Loyalty * 70.0 * Seen);
		Mb.WatchMinutes = static_cast<double>(Mb.Streams) * (20.0 + 100.0 * Mb.Loyalty);
		const double Chat = Fake.Next();
		Mb.Messages = static_cast<int>(static_cast<double>(Mb.Streams) * 4.0 * Chat);
		const double Ago = Fake.Next();
		Mb.FirstSeen = World0 - (10.0 + 100.0 * Ago) * ss::net::MinutesPerDay;
		Mb.LastSeen = World0 - (1.0 + (1.0 - Mb.Loyalty) * (1.0 - Mb.Loyalty) * 60.0 * Ago) * ss::net::MinutesPerDay;
		Mb.Follower = Fake.Chance(0.75);
		Ch.Members.push_back(Mb);
	}
	for (const ss::kast::Member& Mb : Ch.Members)
	{
		if (!Mb.Friend && Mb.Loyalty > 0.55 && Ch.Mods.size() < 2)
		{
			ss::kast::Moderator Md;
			Md.Name = Mb.Name;
			Md.Online = 1.0;
			Md.Actions = 30 - static_cast<int>(Ch.Mods.size()) * 11;
			Ch.Mods.push_back(Md);
		}
	}
	const std::vector<ss::kast::SmallChannel> Small = ss::kast::Network(World0);
	if (!Small.empty())
	{
		Ch.Goodwill[Small.front().Name] = 2;
		Ch.RaidsOut = 3;
	}
	ss::kast::Deal D;
	D.Id = "overclock";
	D.Since = World0 - 3.0 * 1440.0;
	D.Until = World0 + 27.0 * 1440.0;
	D.EarnedCents = 9800;
	Ch.Deals.push_back(D);
	Ch.Offers.insert("tunnelrat");
	Ch.LastOffline = World0;
	Expect(Ch.Regulars() >= 25 && Ch.Superfans() >= 2, "a channel four months in has a core of regulars");

	// The community page: the ladder, the week, the schedule, the people.
	RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
	Click(254.0f, 30.0f);
	Expect(RL.CurrentKastPage() == ss::ui::RiverLine::KastPage::Community, "the Community tab");
	Emit("kast_community", RL, Now + 1.0);
	Now += 1.0;
	Click(70.0f, 763.0f);
	Expect(Ch.ScheduleDays == (4 | 16 | 64), "a day off the schedule");
	Click(70.0f, 763.0f);
	Click(741.0f, 763.0f);
	Expect(Ch.ScheduleDays == (1 | 4 | 16 | 64) && Ch.ScheduleStart == 21 * 60 + 30, "back on, half an hour later");
	Click(539.0f, 763.0f);
	Expect(Ch.ScheduleStart == 21 * 60, "and back to nine");

	// Live from the lobby, then a tournament.
	RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("kast_offline", RL, Now + 1.0);
	Now += 1.0;
	Click(130.0f, 686.0f);
	Expect(S.Streaming() && S.Stream.Chat.size() >= 1, "Go live starts the stream");
	Now = Step(S, Now, 20.0);
	const std::vector<ss::LobbyEvent> Picks = OpenEvents(S, 1, 3000);
	Expect(Picks.size() == 1, "an event to stream");
	if (Picks.empty())
	{
		return;
	}
	S.RegisterEvent(Picks[0]);
	Expect(S.Stream.Pred.Active, "registering on stream starts a prediction");
	Now = Step(S, Now, 300.0);
	S.Stream.OnMoment(S.Channel, S.StreamInputs(), ss::kast::Moment::WonAllIn, "38,400", 1.6);
	ss::kast::Alert Raid;
	Raid.Kind = ss::kast::AlertKind::Raid;
	Raid.Who = Small.empty() ? std::string("chipleader_carla") : Small.front().Name;
	Raid.Count = 23;
	S.Stream.Alerts.insert(S.Stream.Alerts.begin(), Raid);
	Now = Step(S, Now, 1.2);
	Expect(S.Stream.Chat.size() > 20 && S.Stream.Viewers > 5.0 && S.Stream.RegularsHere > 0, "a live stream: chat, viewers, regulars turning up");
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("kast_studio", RL, Now);
	RL.OpenApp(ss::ui::RiverLine::App::RiverLine, Now);
	S.Tab = ss::RightTab::Stream;
	Emit("table_live", RL, Now + 0.1);
	RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Channel);
	Emit("kast_channel", RL, Now + 1.0);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Browse);
	Emit("kast_browse", RL, Now + 1.0);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Community);
	Emit("kast_community_live", RL, Now + 1.0);
	const ss::Chips Bank = S.BankrollCents;
	const ss::Chips Owed = S.Channel.UnpaidCents;
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("kast_studio_end", RL, Now + 1.0);
	const std::vector<ss::kast::SmallChannel> Out = ss::kast::Network(S.WorldMinutes());
	if (Out.empty())
	{
		S.EndStream();
	}
	else
	{
		Click(130.0f, 715.0f);
	}
	Expect(Out.empty() || (S.Stream.Last.RaidedOut.size() > 0 && S.Channel.Goodwill.count(S.Stream.Last.RaidedOut) == 1), "Raid & end sends the viewers to a small channel");
	Expect(!S.Streaming() && S.StreamCard && S.BankrollCents == Bank + Owed && S.Channel.UnpaidCents == 0, "ending the stream pays the balance to the bank");
	Expect(Owed == 0 || (S.Life.Ledger.front().Kind == 6 && S.Life.Ledger.front().Amount == Owed), "the payout is in the ledger");
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("kast_summary", RL, Now + 1.5);
}
} // namespace ui_test

namespace ui_test
{
/** The LED room kit: bought, picked from the setup panel and the Prism card, switched off and on, synced to the stream. */
void LedScreens()
{
	QuietHooks H;
	ss::Session S(H, "ui-leds");
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 600000;
	double Now = 1.0;
	auto Frame = [&](float X, float Y, bool Down, bool Pressed, bool Released) {
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
	};
	auto Click = [&](float X, float Y) {
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	};
	for (const char* Id : {"ram-32", "webcam-1080", "mic-usb", "key-lights", "monitor-24", "headphones", "plant"})
	{
		Expect(S.Buy(Id).empty(), "a streaming desk");
	}
	const double Before = S.GearFx().Quality;
	Expect(!S.RoomGlow(Now).On && !S.GearFx().Leds, "no kit, no glow");
	Expect(S.Buy(ss::gear::LedKitId).empty(), "GearDrop sells the LED kit");
	Expect(S.RoomGlow(Now).On && S.GearFx().Leds && S.GearFx().Quality > Before, "the kit lights up the room (and the stream) when it arrives");
	Expect(!H.Texts.empty() && H.Texts.back().first == "Dee", "Dee sees the window glowing");
	RL.ShowStoreCategory(static_cast<int>(ss::gear::Category::Home));
	RL.OpenApp(ss::ui::RiverLine::App::GearDrop, Now);
	Emit("leds_store", RL, Now + 1.0);
	Now += 1.0;
	RL.UI.Ptr.Active = false;
	// A swatch on the setup panel, then the full controls.
	Click(1120.0f + 132.0f + 27.0f * 3.0f, 402.0f);
	Expect(S.Leds.Preset == 3, "a swatch on the setup panel picks Heater Red");
	Click(1508.0f, 402.0f);
	Expect(RL.RoomLightsShown(), "Customize opens the Prism card");
	Now += 0.6;
	const std::vector<ss::gear::LedPreset>& Presets = ss::gear::LedPresets();
	Expect(Presets.size() == 7, "seven looks");
	for (int K = 0; K < ss::gear::LedPresetCount; ++K)
	{
		Click(900.0f, 286.0f + 58.0f * static_cast<float>(K));
		Expect(S.Leds.Preset == K && S.GearFx().LedPreset == K && S.RoomGlow(Now).Rgb == ss::gear::LedColor(K, Now), "picking a colour lights the room in it");
		RL.UI.Ptr.Active = false;
		Emit("leds_" + Presets[static_cast<size_t>(K)].Id, RL, Now + 0.4);
	}
	Expect(S.RoomGlow(1.0).Rgb != S.RoomGlow(5.0).Rgb, "Aurora drifts");
	Click(1244.0f, 223.0f);
	Expect(!S.Leds.On && !S.RoomGlow(Now).On && !S.GearFx().Leds && S.GearFx().Quality == Before, "the power switch turns the room dark again");
	RL.UI.Ptr.Active = false;
	Emit("leds_off", RL, Now + 0.4);
	Click(1244.0f, 223.0f);
	Expect(S.Leds.On, "and back on");
	Click(846.0f, 700.0f);
	Expect(!S.Leds.Sync, "sync off");
	Click(846.0f, 700.0f);
	Expect(S.Leds.Sync, "sync on");
	Click(80.0f, 500.0f);
	Expect(!RL.RoomLightsShown(), "a click outside closes the card");
	// On stream: the facecam in the room's colour, the chip in the studio, a flash on a big hand.
	S.SetLedPreset(0);
	Expect(S.GoLive().empty() && S.Streaming(), "live");
	Now = Step(S, Now, 30.0);
	RL.OpenApp(ss::ui::RiverLine::App::Kast, Now);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("leds_kast_live", RL, Now);
	Click(1010.0f, 748.0f);
	Expect(RL.RoomLightsShown(), "the studio's lights chip opens the card");
	Click(1201.0f, 769.0f);
	Expect(!RL.RoomLightsShown(), "Done closes it");
	S.SetLedPreset(1);
	const ss::gear::Glow Calm = S.RoomGlow(Now);
	S.Stream.OnMoment(S.Channel, S.StreamInputs(), ss::kast::Moment::WonAllIn, "38,400", 1.6);
	const ss::gear::Glow Win = S.RoomGlow(Now + 0.2);
	Expect(Win.Rgb != Calm.Rgb && Win.Level > Calm.Level, "a won all-in sweeps the room gold");
	RL.UI.Ptr.Active = false;
	Emit("leds_kast_flash", RL, Now + 0.2);
	S.SetLedSync(false);
	Expect(S.RoomGlow(Now + 0.2).Rgb == ss::gear::LedColor(1, Now + 0.2) && S.RoomGlow(Now + 0.2).Level == 1.0, "unsynced, the room holds its colour");
	S.EndStream();
}
/** The living world on RiverLine: boards, news and player cards two months into a career. */
void WorldScreens()
{
	using Page = ss::ui::RiverLine::Page;
	QuietHooks H;
	ss::Session S(H, "ui-world");
	ss::ui::RiverLine RL(S);
	S.CurrentScreen = ss::Screen::Lobby;
	RL.UI.Ptr.Active = true;
	RL.UI.Ptr.X = -1.0f;
	RL.UI.Ptr.Y = -1.0f;
	double Now = Run(S, RL, 10.0, 0.5);
	S.WorldSkip(61);
	const ss::world::World& W = S.Living();
	Expect(ss::net::Shared().Attached() == &W, "the boards read the world");
	RL.ShowBoard(ss::net::Board::Season, Now);
	RL.OpenPage(Page::Leaderboards, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_boards", RL, Now);
	RL.ShowBoard(ss::net::Board::Live, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_boards_live", RL, Now);
	RL.OpenPage(Page::News, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_news", RL, Now);
	// Player cards: the rival, and someone from the Riverside.
	RL.OpenPage(Page::Leaderboards, Now);
	RL.ShowPlayer(W.Find(ss::RivalName), Now);
	Now = Run(S, RL, Now, 1.0);
	Expect(RL.PlayerShown() == W.Find(ss::RivalName), "a player card opens");
	Emit("world_card_rival", RL, Now);
	S.Living().Remember(W.Find("Mei"), ss::world::MemoryKind::Riverside, "the Riverside Sunday", 0, S.WorldMinutes() - 3000.0);
	S.Living().Remember(W.Find("Mei"), ss::world::MemoryKind::HeroKnockedOut, "Riverside Sunday $150", 0, S.WorldMinutes() - 2900.0);
	RL.ShowPlayer(W.Find("Mei"), Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("world_card_mei", RL, Now);
	// A click outside closes it.
	RL.UI.Ptr.X = 60.0f;
	RL.UI.Ptr.Y = 950.0f;
	RL.UI.Ptr.Pressed = true;
	RL.UI.Ptr.Down = true;
	Now = Run(S, RL, Now, 0.05);
	RL.UI.Ptr.Pressed = false;
	RL.UI.Ptr.Down = false;
	RL.UI.Ptr.Released = true;
	Now = Run(S, RL, Now, 0.05);
	RL.UI.Ptr.Released = false;
	Expect(RL.PlayerShown() < 0, "clicking outside closes the card");
	// Who's playing tonight: the event panel lists the regulars registered.
	RL.OpenPage(Page::Lobby, Now);
	std::string Busy;
	size_t Most = 0;
	for (const ss::net::EventInstance& E : ss::net::Shared().Window(S.WorldMinutes(), S.WorldMinutes() + 240.0))
	{
		const size_t N = S.Living().Registered(E.Id).size();
		if (N > Most && ss::net::Shared().TemplateOf(E).BuyInCents >= 1000)
		{
			Most = N;
			Busy = E.Id;
		}
	}
	RL.SelectEvent(Busy);
	RL.ShowEventTab(2);
	Now = Run(S, RL, Now, 1.0);
	Emit("world_lobby_players", RL, Now);
	Expect(Most > 0, "tonight's events show who's registered");
	// A newcomer's card: how they got here, and every step since.
	int Fresh = -1;
	size_t Steps = 0;
	for (const ss::world::Npc& N : W.People())
	{
		if (N.Came != ss::world::Arrival::None && N.Playing() && N.Path.size() > Steps)
		{
			Steps = N.Path.size();
			Fresh = N.Id;
		}
	}
	Expect(Fresh >= 0 && Steps >= 3, "newcomers have joined and started their journeys");
	RL.ShowPlayer(Fresh, Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("world_card_newcomer", RL, Now);
	RL.ShowCardTab(1);
	Now = Run(S, RL, Now, 1.2);
	Expect(!W.ProfileOf(Fresh).Journey.empty() && !W.ProfileOf(Fresh).Came.empty(), "a newcomer's card tells their journey");
	Emit("world_card_journey", RL, Now);
	RL.ShowPlayer(-1, Now);
	// A week on: December's series is running.
	S.WorldSkip(8);
	const ss::net::SeriesInfo* December = ss::net::Shared().CurrentSeries(S.WorldMinutes());
	Expect(December && ss::net::DayOf(S.WorldMinutes()) >= December->FirstDay && December->Id == "hol26", "a series runs in December");
	RL.ShowSeries(December ? December->Id : std::string(), Now);
	RL.OpenPage(Page::Series, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_series_december", RL, Now);
	// The next one on the calendar: what to circle.
	const ss::net::SeriesInfo* Upcoming = nullptr;
	for (const ss::net::SeriesInfo& Sr : ss::net::Shared().Series())
	{
		if (December && Sr.FirstDay > December->LastDay && (!Upcoming || Sr.FirstDay < Upcoming->FirstDay))
		{
			Upcoming = &Sr;
		}
	}
	Expect(Upcoming != nullptr, "another series is on the calendar");
	RL.ShowSeries(Upcoming ? Upcoming->Id : std::string(), Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_series_upcoming", RL, Now);
	RL.OpenPage(Page::Lobby, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_home_series", RL, Now);
	// A year and a half on: the season, the best-known player, the rival.
	S.WorldSkip(420);
	RL.ShowBoard(ss::net::Board::Season, Now);
	RL.OpenPage(Page::Leaderboards, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_boards_later", RL, Now);
	RL.ShowPlayer(W.Leaders(ss::world::Rep::Overall, 1).front(), Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("world_card_leader", RL, Now);
	RL.ShowPlayer(W.Find(ss::RivalName), Now);
	Now = Run(S, RL, Now, 1.0);
	Emit("world_card_rival_later", RL, Now);
	RL.ShowPlayer(-1, Now);
	RL.OpenPage(Page::News, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_news_later", RL, Now);
	std::printf("%s", W.Describe(W.Find("Mei")).c_str());
	// June: The Championship Online's bracelet events, and someone who has won one.
	const int June = ss::world::DayOn(ss::world::YearOf(ss::net::DayOf(S.WorldMinutes())), 6, 14);
	S.WorldSkip(June - ss::net::DayOf(S.WorldMinutes()));
	const ss::net::SeriesInfo* Bracelets = ss::net::Shared().CurrentSeries(S.WorldMinutes());
	Expect(Bracelets && Bracelets->Bracelets > 0 && ss::net::DayOf(S.WorldMinutes()) >= Bracelets->FirstDay, "June brings The Championship Online");
	RL.ShowSeries(Bracelets ? Bracelets->Id : std::string(), Now);
	RL.OpenPage(Page::Series, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_series_bracelets", RL, Now);
	int Wearer = -1;
	for (const ss::world::Npc& N : W.People())
	{
		for (const ss::world::Step& St : N.Path)
		{
			if ((St.Kind == ss::world::StepKind::Bracelet || St.Kind == ss::world::StepKind::Ring) && (Wearer < 0 || N.Path.size() > W.Get(Wearer)->Path.size()))
			{
				Wearer = N.Id;
			}
		}
	}
	Expect(Wearer >= 0, "someone has won an online bracelet or ring");
	if (Wearer >= 0)
	{
		RL.ShowPlayer(Wearer, Now);
		RL.ShowCardTab(1);
		Now = Run(S, RL, Now, 1.2);
		Emit("world_card_bracelet", RL, Now);
		RL.ShowPlayer(-1, Now);
	}
	// At the table: a name the player hasn't seen before catches their eye, and opens its card.
	const std::vector<ss::LobbyEvent> Open = OpenEvents(S, 1, 3000);
	Expect(!Open.empty(), "an event to sit down at");
	if (!Open.empty())
	{
		S.BankrollCents = std::max<ss::Chips>(S.BankrollCents, Open[0].BuyInCents + 1000);
		S.RegisterEvent(Open[0]);
		Now = Run(S, RL, Now, 3.0);
		int Face = -1;
		if (S.T)
		{
			const int Mine = S.T->Hero().TableId;
			for (const ss::TPlayer& P : S.T->Players)
			{
				const auto It = S.FieldNpc.find(P.Id);
				if (P.TableId == Mine && It != S.FieldNpc.end() && (Face < 0 || W.Get(It->second)->Came != ss::world::Arrival::None))
				{
					Face = It->second;
				}
			}
		}
		Expect(Face >= 0, "people the world knows sit at the player's table");
		if (Face >= 0)
		{
			RL.ShowPlayer(Face, Now);
			RL.ShowCardTab(1);
			Now = Run(S, RL, Now, 1.2);
			Expect(RL.PlayerShown() == Face, "a card opens over the table");
			Emit("world_table_card", RL, Now);
		}
	}
}


// ------------------------------------------------------------------ event art

void SaveSheet(const std::string& Name, const ss::ui::DrawList& L)
{
	Expect(!L.Cmds.empty() && L.Vertices.size() > 1000, (Name + " drew something").c_str());
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

void SheetBackground(ss::ui::Canvas& C, const std::string& Title, const std::string& Sub)
{
	C.FillRect({0.0f, 0.0f, 1600.0f, 1000.0f}, ss::ui::Paint::Linear({0.0f, 0.0f}, {0.0f, 1000.0f}, ss::ui::Hex(0x111a2b), ss::ui::Hex(0x070b14)));
	C.Text(Title, 48.0f, 62.0f, ss::ui::Ts(30.0f, 900, ss::ui::Hex(0xffffff)));
	C.Text(Sub, 48.0f, 92.0f, ss::ui::Ts(16.0f, 500, ss::ui::Hex(0x8b9bb4)));
}

void EventArtGallery()
{
	namespace ea = ss::ui::eventart;
	const ss::net::Network& Net = ss::net::Shared();
	TableMeasurer M;
	const double Time = 3.2;
	// Every glyph.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "EVENT ART \xC2\xB7 GLYPHS", "Every motif, white on the night");
		const int Count = static_cast<int>(ea::Glyph::Count);
		for (int K = 0; K < Count; ++K)
		{
			const float X = 90.0f + static_cast<float>(K % 12) * 128.0f;
			const float Y = 170.0f + static_cast<float>(K / 12) * 134.0f;
			C.FillRoundRect({X - 52.0f, Y - 52.0f, 104.0f, 104.0f}, 22.0f, ss::ui::Hex(0x18243a));
			ea::DrawGlyph(C, static_cast<ea::Glyph>(K), X, Y, 72.0f);
		}
		SaveSheet("eventart_glyphs", L);
	}
	// Every tournament on the schedule, as the lobby shows it.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "EVENT ART \xC2\xB7 THE SCHEDULE", "Every tournament brand on RiverLine has its own tile");
		std::set<std::string> Seen;
		int K = 0;
		for (const ss::net::EventTemplate& T : Net.Templates())
		{
			if (!T.Series.empty())
			{
				continue;
			}
			std::string Key = T.Name;
			if (!Seen.insert(Key).second && T.Id.rfind("step", 0) != 0)
			{
				continue;
			}
			const float X = 110.0f + static_cast<float>(K % 9) * 172.0f;
			const float Y = 180.0f + static_cast<float>(K / 9) * 150.0f;
			ea::Emblem(C, T, X, Y, 84.0f, Time);
			C.Text(T.Name, X, Y + 66.0f, ss::ui::Ts(12.0f, 700, ss::ui::Hex(0xc3cedf), ss::ui::Align::Center, ss::ui::Baseline::Alphabetic, false, 160.0f));
			++K;
		}
		Expect(K >= 40, "every brand on the schedule has a tile");
		SaveSheet("eventart_tiles", L);
	}
	// The series: one year's crests.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "EVENT ART \xC2\xB7 THE SERIES", "Each series wears its own crest; the flagships return every year, the seasonal ones change their names");
		int K = 0;
		for (const ss::net::SeriesInfo& Sr : Net.Series())
		{
			const int Year = ss::world::YearOf(Sr.FirstDay);
			if (!(Year == 2027 || Sr.Id == "hol26" || Sr.Id == "rcop" || Sr.Id == "mm") || K >= 12)
			{
				continue;
			}
			const float X = 150.0f + static_cast<float>(K % 6) * 260.0f;
			const float Y = 300.0f + static_cast<float>(K / 6) * 380.0f;
			ea::SeriesCrest(C, Sr, X, Y, 190.0f, Time + K);
			C.Text(Sr.Name, X, Y + 150.0f, ss::ui::Ts(15.0f, 800, ss::ui::Hex(0xffffff), ss::ui::Align::Center));
			C.Text(ss::net::DateLabel(Sr.FirstDay) + " \xE2\x80\x93 " + ss::net::DateLabel(Sr.LastDay) + ", " + std::to_string(Year), X, Y + 172.0f,
				ss::ui::Ts(12.0f, 600, ss::ui::Hex(0x8b9bb4), ss::ui::Align::Center));
			++K;
		}
		SaveSheet("eventart_series", L);
	}
	// Series events: what makes one matter, at the sizes the screens use.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "EVENT ART \xC2\xB7 SERIES EVENTS", "Rims by stakes (bronze, silver, gold, platinum); Main Events, high rollers, bracelets and rings dressed for the occasion");
		auto Find = [&](const std::string& Id) -> const ss::net::EventTemplate* { return Net.FindTemplate(Id); };
		std::vector<std::pair<const ss::net::EventTemplate*, std::string>> Show;
		auto Add = [&](const ss::net::EventTemplate* T, const std::string& Label) {
			if (T)
			{
				Show.push_back({T, Label});
			}
		};
		Add(Find("rcop27-main"), "RCOP Main Event");
		Add(Find("tco27-main"), "Online Championship (bracelet)");
		Add(Find("ring27-main"), "Ring Main Event");
		Add(Find("slam27-mini"), "Mini Main Event");
		Add(Find("hrs27-shr"), "Super High Roller");
		Add(Find("hol26-main"), "Holiday Heist Main");
		// A bracelet event, a ring event, and one plain event at each stake.
		const ss::net::EventTemplate* Bracelet = nullptr;
		const ss::net::EventTemplate* Ring = nullptr;
		const ss::net::EventTemplate* ByTier[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
		for (const ss::net::EventTemplate& T : Net.Templates())
		{
			if (T.Series.rfind("tco27", 0) == 0 && T.Bracelet && !T.Main && !Bracelet)
			{
				Bracelet = &T;
			}
			if (T.Series.rfind("ring27", 0) == 0 && T.Ring && !T.Main && !Ring)
			{
				Ring = &T;
			}
			if (T.Series.rfind("win27", 0) == 0 && !T.Main && !T.Featured)
			{
				const int Tr = static_cast<int>(ss::net::TierOf(T.BuyInCents));
				ByTier[Tr] = ByTier[Tr] ? ByTier[Tr] : &T;
			}
		}
		Add(Bracelet, "Bracelet event");
		Add(Ring, "Ring event");
		Add(ByTier[1], "Micro (bronze)");
		Add(ByTier[2], "Low (silver)");
		Add(ByTier[3], "Mid (gold)");
		Add(Find("spr27-hr"), "High Roller");
		for (size_t I = 0; I < Show.size(); ++I)
		{
			const float X = 150.0f + static_cast<float>(I % 6) * 260.0f;
			const float Y = 300.0f + static_cast<float>(I / 6) * 380.0f;
			ea::Emblem(C, *Show[I].first, X, Y, 190.0f, Time + static_cast<double>(I));
			C.Text(Show[I].second, X, Y + 150.0f, ss::ui::Ts(15.0f, 800, ss::ui::Hex(0xffffff), ss::ui::Align::Center));
			C.Text(Show[I].first->Name, X, Y + 172.0f, ss::ui::Ts(11.0f, 600, ss::ui::Hex(0x8b9bb4), ss::ui::Align::Center, ss::ui::Baseline::Alphabetic, false, 250.0f));
			// The same emblem at row size.
			ea::Emblem(C, *Show[I].first, X + 100.0f, Y - 130.0f, 40.0f, Time);
		}
		Expect(Show.size() == 12, "the special events are all there");
		SaveSheet("eventart_crests", L);
	}
}
} // namespace ui_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	ui_test::NetScreens();
	ui_test::WorldScreens();
	ui_test::AppScreens();
	ui_test::Clicks();
	ui_test::Screens();
	ui_test::Results();
	ui_test::Props();
	ui_test::Avatars();
	ui_test::EventArtGallery();
	ui_test::MultiScreens();
	ui_test::StreamGallery();
	ui_test::StreamScreens();
	ui_test::LedScreens();
	ui_test::FrontEndFlows();
	ui_test::FrontEndScreens();
	if (ui_test::Failures == 0)
	{
		std::printf("ui tests: all passed\n");
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
