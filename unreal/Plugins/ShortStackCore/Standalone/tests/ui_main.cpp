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
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/StoreCounter.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/PropArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/StreamArt.h"
#include "TestFontMetrics.h"

#include <cstdio>
#include <functional>
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
	ss::hero::Character Who;
	void UiSound(ss::SoundId, double) override { ++Sounds; }
	void Continue() override { ++Continues; }
	void NewGame(const std::string& Name) override { NewGameName = Name; }
	void NewCareer(const std::string& Name, const ss::hero::Character& Person) override
	{
		NewGameName = Name;
		Who = Person;
	}
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
	using Step = ss::ui::FrontEnd::CreatorStage;
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::NewGame && Fe.CreatorStep() == Step::Identity && Fe.Draft.Problem().empty(), "Enter on New Game opens the creator with someone to start from");
	auto Type = [&](const std::string& Text, bool Clear = true) {
		for (int I = 0; Clear && I < 20; ++I)
		{
			Fe.Key("Backspace", Now);
		}
		for (const char Ch : Text)
		{
			// Unreal sends Q, E and R as keys too: typing them must not switch tabs or reroll.
			if (Ch == 'R' || Ch == 'Q' || Ch == 'E')
			{
				Fe.Key(std::string(1, Ch), Now);
			}
			Fe.Char(static_cast<uint32_t>(static_cast<unsigned char>(Ch)), Now);
		}
	};
	Type("");
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::NewGame && Fe.CreatorStep() == Step::Identity && H.NewGameName.empty() && Fe.CreatorField() == 0, "an empty first name is refused");
	const ss::hero::Look Before = Fe.Draft.Appearance;
	Type("Rosa Mar");
	Fe.Char(0xED, Now); // i with an acute accent
	Type("a", false);
	Fe.Char(0xED, Now);
	Expect(Fe.Draft.FirstName == "Rosa Mar\xC3\xAD" "a\xC3\xAD" && Fe.Draft.Appearance == Before, "names take spaces and accents; typing R doesn't reroll");
	Fe.Key("Backspace", Now);
	Expect(Fe.Draft.FirstName == "Rosa Mar\xC3\xAD" "a", "backspace removes a whole accented letter");
	Fe.Key("Tab", Now);
	Type("Delgado");
	Fe.Key("Tab", Now);
	Type("ace high!");
	Expect(Fe.ScreenName == "acehigh", "only valid screen name characters are typed");
	Fe.Key("Tab", Now);
	const int Age = Fe.Draft.Age;
	Fe.Key(Age > 30 ? "Left" : "Right", Now);
	Expect(Fe.CreatorField() == 3 && Fe.Draft.Age == Age + (Age > 30 ? -1 : 1), "Left and Right change the age");
	Fe.Key("Down", Now);
	const std::string Country = Fe.Draft.Country;
	Fe.Key("Right", Now);
	Expect(Fe.Draft.Country != Country, "the country grid moves");
	Fe.Key("Enter", Now);
	Expect(Fe.CreatorStep() == Step::Background, "a complete identity moves on to the background");
	const ss::hero::Background Was = Fe.Draft.Story;
	Fe.Key("Right", Now);
	Expect(Fe.Draft.Story != Was, "arrows choose a background");
	const ss::hero::Background Chosen = Fe.Draft.Story;
	Fe.Key("Enter", Now);
	Expect(Fe.CreatorStep() == Step::Look && Fe.CreatorTab() == 0, "then the look, on the face tab");
	Fe.Key("E", Now);
	const int Hair = Fe.Draft.Appearance.Hair;
	Fe.Key("Right", Now);
	Expect(Fe.CreatorTab() == 1 && Fe.Draft.Appearance.Hair == (Hair + 1) % ss::hero::OptionCount(ss::hero::Slot::Hair), "E opens the hair tab and Right changes the style");
	Fe.Key("Q", Now);
	Fe.Key("Q", Now);
	Fe.Key("Down", Now);
	const int Height = Fe.Draft.Appearance.Height;
	Fe.Key("Right", Now);
	Expect(Fe.CreatorTab() == 3 || (Fe.CreatorTab() == 2 && Fe.Draft.Appearance.Height == std::min(ss::hero::MaxHeight, Height + 1)), "the body tab sets the height");
	Fe.Key("Enter", Now);
	Expect(Fe.CreatorStep() == Step::Review, "then the review");
	Fe.Key("Escape", Now);
	Expect(Fe.CreatorStep() == Step::Look, "Escape steps back");
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Expect(H.NewGameName == "acehigh" && !Fe.IsOpen() && H.Who.FirstName == "Rosa Mar\xC3\xAD" "a" && H.Who.LastName == "Delgado" && H.Who.Story == Chosen && H.Who.Created,
		"Begin starts a new career as the person made");

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
	Fe.Info.Person.Created = true;
	Fe.Info.Person.Story = ss::hero::Background::Dropout;
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
	for (int I = 0; I < 20; ++I)
	{
		Fe.Key("Backspace", Now);
	}
	for (const char Ch : std::string("Rosa"))
	{
		Fe.Char(static_cast<uint32_t>(Ch), Now);
	}
	Fe.Key("Tab", Now);
	for (int I = 0; I < 20; ++I)
	{
		Fe.Key("Backspace", Now);
	}
	for (const char Ch : std::string("Delgado"))
	{
		Fe.Char(static_cast<uint32_t>(Ch), Now);
	}
	Fe.Draft.Country = "MX";
	Fe.Draft.Age = 31;
	Fe.Key("Down", Now);
	Fe.Key("Down", Now);
	Fe.Key("Down", Now);
	Now = Settle(Fe, Now, 0.5);
	EmitMenu("menu_creator_identity", Fe, Now);
	Fe.Key("Enter", Now);
	Fe.Draft.Story = ss::hero::Background::Kitchen;
	Now = Settle(Fe, Now, 0.8);
	EmitMenu("menu_creator_background", Fe, Now);
	Fe.Key("Enter", Now);
	Fe.Draft.Appearance.Body = 1;
	Fe.Draft.Appearance.Face = 4;
	Fe.Draft.Appearance.Skin = 5;
	Fe.Draft.Appearance.Hair = 8;
	Fe.Draft.Appearance.HairColor = 0;
	Fe.Draft.Appearance.FacialHair = 0;
	Fe.Draft.Appearance.Outfit = 4;
	Fe.Draft.Appearance.OutfitColor = 5;
	Fe.Draft.Appearance.Glasses = 0;
	Fe.Draft.Appearance.Hat = 0;
	Fe.Draft.Appearance.Height = 168;
	Fe.Key("Down", Now);
	Fe.Key("Down", Now);
	Now = Settle(Fe, Now, 0.8);
	EmitMenu("menu_creator_look", Fe, Now);
	Fe.Key("E", Now);
	Fe.Key("E", Now);
	Now = Settle(Fe, Now, 0.8);
	EmitMenu("menu_creator_body", Fe, Now);
	Fe.Key("E", Now);
	Now = Settle(Fe, Now, 0.8);
	EmitMenu("menu_creator_style", Fe, Now);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 1.0);
	EmitMenu("menu_creator_review", Fe, Now);
	EmitMenu("menu_creator_review_ultrawide", Fe, Now + 0.05, 2520.0f);
	Fe.Key("Escape", Now);
	Fe.Key("Escape", Now);
	Fe.Key("Escape", Now);
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
	// A night shift at the Lucky Penny: the time-lapse, then the result.
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
	Expect(S.TimeSkip.Active && S.TimeSkip.Result.ActivityId == "quikstop", "Take shift starts the Lucky Penny shift");
}
/** Penny Drop on the laptop: the shelves, a cart, an order on its way, eating from the bag, the kitchen tap, the order arriving. */
void PennyDropScreens()
{
	QuietHooks H;
	ss::Session S(H, "ui-drop");
	ss::ui::RiverLine RL(S);
	TableMeasurer M;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 4280;
	S.Life.Hunger = 72.0;
	S.Life.Thirst = 66.0;
	S.Life.Energy = 41.0;
	S.Life.Pantry["oodle-cup"] = 2;
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
	using App = ss::ui::RiverLine::App;
	RL.UI.Ptr.Active = false;
	RL.OpenApp(App::PennyDrop, Now);
	Emit("drop_app", RL, Now + 1.0);
	Now += 1.0;
	// Two waters and a cola from the cooler, a hot dog from the grill.
	Click(276.0f, 377.0f);
	Click(276.0f, 377.0f);
	Click(592.0f, 377.0f);
	RL.ShowDropShelf(static_cast<int>(ss::store::Shelf::Hot));
	Frame(0.0f, 0.0f, false, false, false);
	Now += 0.6;
	Click(276.0f, 377.0f);
	Expect(RL.DropBasket().Count() == 4 && RL.DropBasket().Lines.size() == 3, "Penny Drop: four things in the cart");
	RL.UI.Ptr.Active = false;
	Emit("drop_cart", RL, Now + 0.3);
	Now += 0.3;
	const ss::Chips Total = ss::store::DeliveryTotal(RL.DropBasket());
	Click(1290.0f, 512.0f);
	Expect(S.Life.Deliveries.size() == 1 && RL.DropBasket().Empty() && S.BankrollCents == 4280 - Total, "Penny Drop: placing the order pays for it and clears the cart");
	S.LobbyMinutes += 18.0;
	S.Update(Now);
	// Eat a noodle cup from the bag while you wait, and have a glass from the tap.
	const double Hunger = S.Life.Hunger;
	Click(580.0f, 798.0f);
	Expect(S.Life.Hunger < Hunger - 20.0 && S.Life.Pantry["oodle-cup"] == 1, "Penny Drop: eating from the bag at home");
	const double Thirst = S.Life.Thirst;
	Click(268.0f, 897.0f);
	Expect(S.Life.Thirst < Thirst - 10.0, "Penny Drop: a glass of tap water");
	RL.UI.Ptr.Active = false;
	Emit("drop_tracking", RL, Now + 0.2);
	Now += 0.2;
	S.LobbyMinutes += 60.0;
	S.Update(Now);
	Expect(S.Life.Deliveries.empty() && S.Life.Pantry["cascade"] == 2 && S.Life.Pantry["roller-dog"] == 1, "Penny Drop: the order arrives in the bag");
	Emit("drop_home", RL, Now + 0.2);
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
	// The champions: whoever has won the most bracelets and rings, on their card and in their trophy case.
	int Champ = -1;
	size_t Kept = 0;
	for (const ss::world::Npc& N : W.People())
	{
		Expect(static_cast<int>(N.Awards.size()) == N.Bracelets + N.Rings, "every bracelet and ring is in its winner's trophy case");
		if (N.Awards.size() > Kept)
		{
			Champ = N.Id;
			Kept = N.Awards.size();
		}
	}
	Expect(Champ >= 0, "someone has bracelets or rings to show");
	if (Champ >= 0)
	{
		RL.ShowPlayer(Champ, Now);
		RL.ShowCardTab(0);
		Now = Run(S, RL, Now, 1.2);
		Emit("world_card_champion", RL, Now);
		RL.ShowCardTab(2);
		Now = Run(S, RL, Now, 1.2);
		Emit("world_card_trophies", RL, Now);
		// RiverLine Stats: their profit graph, ROI by buy-in and format, finishes and records.
		RL.ShowCardTab(3);
		Now = Run(S, RL, Now, 1.5);
		const ss::world::Tracker& T = W.Get(Champ)->Stats;
		const ss::world::Npc& Cn = *W.Get(Champ);
		Expect(T.Events == Cn.Totals[0].Events + Cn.Totals[1].Events, "the stats page counts every tournament");
		Expect(T.BuyIns == Cn.Totals[0].Spent + Cn.Totals[1].Spent && T.Prizes >= Cn.Totals[0].Won + Cn.Totals[1].Won, "buy-ins and prizes add up");
		Expect(!T.Curve.empty() && static_cast<int>(T.Curve.size()) < ss::world::Tracker::CurveMax, "the profit graph has its points");
		Emit("world_card_stats", RL, Now);
		// Hovering the graph: the crosshair and what it says.
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = 700.0f;
		RL.UI.Ptr.Y = 560.0f;
		Now = Run(S, RL, Now, 0.2);
		Emit("world_card_stats_hover", RL, Now);
		// The ABI view: how their buy-ins moved, against the stakes.
		RL.UI.Ptr.Active = false;
		RL.ShowStatsGraph(1);
		Now = Run(S, RL, Now, 1.5);
		Expect(T.Spend.size() == T.Curve.size() && (T.Spend.empty() || T.Spend.back() <= T.BuyIns), "the ABI graph has its points");
		Emit("world_card_stats_abi", RL, Now);
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = 640.0f;
		RL.UI.Ptr.Y = 560.0f;
		Now = Run(S, RL, Now, 0.2);
		Emit("world_card_stats_abi_hover", RL, Now);
		RL.ShowStatsGraph(0);
		RL.UI.Ptr.Active = false;
		RL.ShowPlayer(-1, Now);
	}
	// The network's biggest winner, on the same page.
	{
		int Top = -1;
		for (const ss::world::Npc& N : W.People())
		{
			Top = !N.Faded && (Top < 0 || N.Stats.Net > W.Get(Top)->Stats.Net) ? N.Id : Top;
		}
		Expect(Top >= 0 && W.Get(Top)->Stats.Net > 0, "someone is up on the network");
		RL.ShowPlayer(Top, Now);
		RL.ShowCardTab(3);
		Now = Run(S, RL, Now, 1.5);
		Emit("world_card_stats_winner", RL, Now);
		RL.ShowPlayer(-1, Now);
	}
	// The boards: champions wear their frames there too.
	RL.ShowBoard(ss::net::Board::Earnings, Now);
	RL.OpenPage(Page::Leaderboards, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_boards_champions", RL, Now);
	// The player's own: a bracelet and a Main Event ring in the trophy case on the Career page.
	S.Living().GrantHeroResults(260);
	S.Living().GrantAward(-1, false, false);
	S.Living().GrantHeroResults(180);
	S.Living().GrantAward(-1, true, true);
	S.Living().GrantHeroResults(90);
	Expect(W.HeroAwards().size() == 2, "the player's trophy case holds what they won");
	Expect(W.HeroStats().Events >= 530 && W.HeroAwards().front().Tourney == 260, "the player's stats page, with their titles on the graph");
	RL.OpenPage(Page::Career, Now);
	Now = Run(S, RL, Now, 1.5);
	Emit("world_career_trophies", RL, Now);
	// Their own stats, from the Career page.
	RL.ShowPlayer(ss::ui::RiverLine::HeroCard, Now);
	Now = Run(S, RL, Now, 1.5);
	Expect(RL.CardOpen(), "the player's stats open");
	Emit("world_hero_stats", RL, Now);
	RL.ShowPlayer(-1, Now);
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
		// Champions at the table: a bracelet winner and a ring winner in their frames (the player in theirs).
		if (S.T)
		{
			const int Mine = S.T->Hero().TableId;
			int Given = 0;
			for (const ss::TPlayer& P : S.T->Players)
			{
				const auto It = S.FieldNpc.find(P.Id);
				if (P.TableId == Mine && It != S.FieldNpc.end() && Given < 2)
				{
					S.Living().GrantAward(It->second, Given == 1, false);
					++Given;
				}
			}
			Expect(Given >= 1, "champions to seat at the player's table");
			Now = Run(S, RL, Now, 1.0);
			Emit("world_table_champions", RL, Now);
		}
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

/** The street's printed and glowing things: the store's sign, the OPEN neon, the street blades, the door, a poster. */
void StreetProps()
{
	namespace P = ss::ui::props;
	TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	SheetBackground(C, "Fifth and Market", "The Lucky Penny #212's signs and windows, and the corner's street blades (drawn once, shown on quads in the Street level)");
	auto Place = [&](float X, float Y, float Scale, float W, float H, const std::function<void()>& Paint) {
		C.Save();
		C.Translate(X, Y);
		C.Scale(Scale, Scale);
		C.FillRect({0.0f, 0.0f, W, H}, ss::ui::Paint(ss::ui::Rgba(0, 0, 0, 0.25f)));
		Paint();
		C.Restore();
	};
	Place(48.0f, 120.0f, 0.62f, P::StoreSignW, P::StoreSignH, [&] { P::StoreSign(C); });
	Place(48.0f, 340.0f, 0.62f, P::OpenSignW, P::OpenSignH, [&] { P::OpenSign(C); });
	Place(500.0f, 340.0f, 0.5f, P::StreetSignW, P::StreetSignH, [&] { P::StreetSign(C, "FIFTH ST", "1800"); });
	Place(500.0f, 460.0f, 0.5f, P::StreetSignW, P::StreetSignH, [&] { P::StreetSign(C, "MARKET ST", "200"); });
	Place(500.0f, 580.0f, 0.75f, P::BuildingNumberW, P::BuildingNumberH, [&] { P::BuildingNumber(C, "1812"); });
	Place(48.0f, 540.0f, 0.75f, P::DoorDecalW, P::DoorDecalH, [&] { P::DoorDecal(C); });
	Place(1060.0f, 120.0f, 0.5f, P::PromoW, P::PromoH, [&] { P::Promo(C, "volt-rush", "2 FOR $5"); });
	Place(1330.0f, 120.0f, 0.42f, P::PromoW, P::PromoH, [&] { P::Promo(C, "roller-dog", "$1.99"); });
	Place(1330.0f, 520.0f, 0.42f, P::PromoW, P::PromoH, [&] { P::Promo(C, "night-owl-brew", "NEW"); });
	SaveSheet("street_props", L);
}

/** The Lucky Penny's counter: the shelves, a basket on the receipt, a declined card, and the bag. */
void StoreScreens()
{
	struct NoHooks : ss::SessionHooks
	{
	};
	NoHooks H;
	ss::Session S(H, "store-ui");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 1864;
	S.Life.Hunger = 78.0;
	S.Life.Thirst = 66.0;
	S.Life.Energy = 31.0;
	ss::ui::StoreCounter Counter(S);
	TableMeasurer M;
	auto Emit = [&](const std::string& Name, double Now, float Width = 1920.0f) {
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, Width, ss::ui::StoreCounter::Height, 1.0f);
		C.FillRect({0.0f, 0.0f, Width, 1080.0f}, ss::ui::Paint::Linear({0.0f, 0.0f}, {Width, 1080.0f}, ss::ui::Hex(0x2a3a3c), ss::ui::Hex(0x10161c)));
		Counter.Draw(C, Now);
		Expect(!L.Cmds.empty(), "the counter drew something");
		std::printf("  %-22s %6zu vertices %4zu commands\n", Name.c_str(), L.Vertices.size(), L.Cmds.size());
		WriteList(Name, L);
	};
	double Now = 10.0;
	Counter.Open(Now);
	Expect(Counter.IsOpen() && Counter.Basket.Empty(), "the counter opens with an empty basket");
	Now += 1.0;
	Emit("store_counter", Now);
	Counter.Key("Right", Now);
	Counter.Key("Right", Now);
	Counter.Key("Enter", Now);
	Counter.Key("Enter", Now);
	Counter.Key("E", Now);
	Counter.Key("Enter", Now);
	Counter.Key("E", Now);
	Counter.Key("Enter", Now);
	Expect(Counter.Basket.Count() == 4 && Counter.Shelf() == 2, "arrows, Enter and E fill the basket across shelves");
	Now += 1.0;
	Emit("store_counter_basket", Now);
	Counter.Key("Tab", Now);
	Expect(Counter.Basket.Empty() && S.Life.Pantry.size() == 3, "Tab pays and bags it");
	Now += 0.5;
	Emit("store_counter_paid", Now);
	Counter.Key("Q", Now);
	Counter.Key("Q", Now);
	for (int I = 0; I < 9; ++I)
	{
		Counter.Key("Enter", Now);
	}
	S.BankrollCents = 120;
	Counter.Key("Tab", Now);
	Expect(!Counter.Basket.Empty(), "a declined card keeps the basket");
	Now += 0.5;
	Emit("store_counter_declined", Now);
	Counter.Key("Escape", Now);
	Expect(!Counter.IsOpen() && Counter.TakeLeave() && !Counter.TakeLeave(), "Escape walks away once");

	// The walking HUD over a stand-in street (dark, a neon wash, the store's light spilling out).
	{
		ss::ui::DrawList Lh;
		ss::ui::Canvas Ch(Lh, M, 1920.0f, 1080.0f, 1.0f);
		Ch.FillRect({0.0f, 0.0f, 1920.0f, 1080.0f}, ss::ui::Paint::Linear({0.0f, 0.0f}, {0.0f, 1080.0f}, ss::ui::Hex(0x0b1220), ss::ui::Hex(0x05070b)));
		Ch.FillCircle(1500.0f, 560.0f, 520.0f, ss::ui::Paint::Radial({1500.0f, 560.0f}, 0.0f, {1500.0f, 560.0f}, 520.0f, ss::ui::Rgba(255, 220, 160, 0.22f), 0.5f, ss::ui::Rgba(255, 200, 140, 0.06f), ss::ui::Rgba(0, 0, 0, 0.0f)));
		Ch.FillCircle(420.0f, 420.0f, 460.0f, ss::ui::Paint::Radial({420.0f, 420.0f}, 0.0f, {420.0f, 420.0f}, 460.0f, ss::ui::Rgba(255, 46, 136, 0.16f), 0.5f, ss::ui::Rgba(255, 46, 136, 0.04f), ss::ui::Rgba(0, 0, 0, 0.0f)));
		ss::ui::StreetHudInfo Hud;
		Hud.Place = "FIFTH STREET";
		Hud.Clock = "2:41 AM";
		Hud.BankrollCents = 1864;
		Hud.Life = &S.Life;
		Hud.Prompt = "Go into the Lucky Penny";
		Hud.Toasts.push_back({"Mom", "are you eating? you never answer when I ask if you're eating", 99.0});
		Hud.Toasts.push_back({"Dee", "Tuesday game's on. Bring cash, not excuses.", 100.5});
		Hud.HintsAt = 96.0;
		ss::ui::DrawStreetHud(Ch, Hud, 101.0);
		SaveSheet("street_hud", Lh);
	}

	// The open world's vitals block, over a street.
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	SheetBackground(C, "Vitals and the shelves", "The open world's HUD block, every product's art, and the needs from fed to starving");
	for (int I = 0; I < 3; ++I)
	{
		ss::life::State Lf;
		Lf.Hunger = I == 0 ? 15.0 : I == 1 ? 68.0 : 94.0;
		Lf.Thirst = I == 0 ? 20.0 : I == 1 ? 72.0 : 90.0;
		Lf.Energy = I == 0 ? 88.0 : I == 1 ? 46.0 : 14.0;
		C.FillRoundRect({48.0f + static_cast<float>(I) * 512.0f, 130.0f, 488.0f, 140.0f}, 12.0f, ss::ui::Paint(ss::ui::Rgba(4, 6, 10, 0.7f)));
		ss::ui::DrawVitals(C, Lf, 72.0f + static_cast<float>(I) * 512.0f, 160.0f, 440.0f, 3.0);
	}
	const std::vector<ss::store::Item>& All = ss::store::Catalog();
	for (size_t I = 0; I < All.size(); ++I)
	{
		const float X = 110.0f + static_cast<float>(I % 7) * 220.0f;
		const float Y = 440.0f + static_cast<float>(I / 7) * 300.0f;
		ss::ui::DrawProduct(C, All[I], X, Y, 180.0f);
		C.Text(All[I].Name, X, Y + 128.0f, ss::ui::Ts(16.0f, 800, ss::ui::Hex(0xffffff), ss::ui::Align::Center));
	}
	SaveSheet("store_products", L);
}

/** The character creator's portraits: a cast that covers every hairstyle, face, outfit, hat and pair of glasses. */
void PortraitGallery()
{
	namespace hero = ss::hero;
	using ss::ui::Hex;
	struct Pick
	{
		const char* First;
		int Age;
		hero::Background Story;
		hero::Look L;
	};
	auto Look = [](int Body, int Face, int Skin, int Eyes, int Brows, int Hair, int HairColor, int Facial, int Build, int Outfit, int OutfitColor, int Glasses, int Hat) {
		hero::Look L;
		L.Body = Body;
		L.Face = Face;
		L.Skin = Skin;
		L.Eyes = Eyes;
		L.Brows = Brows;
		L.Hair = Hair;
		L.HairColor = HairColor;
		L.FacialHair = Facial;
		L.Build = Build;
		L.Outfit = Outfit;
		L.OutfitColor = OutfitColor;
		L.Glasses = Glasses;
		L.Hat = Hat;
		return L;
	};
	const Pick Cast[12] = {
		{"Jesse", 24, hero::Background::Newcomer, Look(0, 0, 3, 1, 1, 3, 1, 1, 1, 0, 0, 0, 0)},
		{"Rosa", 31, hero::Background::Kitchen, Look(1, 4, 5, 0, 3, 8, 0, 0, 1, 4, 5, 0, 0)},
		{"Malik", 27, hero::Background::Hustler, Look(0, 1, 8, 1, 2, 1, 0, 4, 2, 1, 2, 0, 0)},
		{"Yui", 22, hero::Background::Dropout, Look(1, 2, 1, 1, 0, 11, 0, 0, 0, 5, 6, 1, 0)},
		{"Sean", 46, hero::Background::Bouncer, Look(0, 5, 1, 4, 2, 5, 4, 5, 3, 3, 1, 0, 0)},
		{"Ama", 29, hero::Background::DealersKid, Look(1, 0, 9, 0, 1, 10, 0, 0, 1, 2, 3, 0, 0)},
		{"Diego", 35, hero::Background::Hustler, Look(0, 3, 5, 0, 1, 6, 0, 3, 1, 4, 5, 4, 0)},
		{"Freya", 26, hero::Background::Dropout, Look(1, 0, 0, 4, 1, 9, 5, 0, 0, 0, 4, 0, 1)},
		{"Kenji", 58, hero::Background::DealersKid, Look(0, 1, 2, 1, 0, 4, 7, 2, 1, 2, 0, 2, 0)},
		{"Nia", 33, hero::Background::Kitchen, Look(1, 4, 7, 2, 3, 7, 0, 0, 2, 1, 7, 0, 0)},
		{"Marco", 41, hero::Background::Bouncer, Look(0, 1, 4, 3, 2, 0, 0, 5, 3, 0, 5, 0, 2)},
		{"Theo", 20, hero::Background::Newcomer, Look(0, 4, 3, 2, 1, 2, 5, 0, 0, 5, 2, 3, 3)},
	};
	const char* const Labels[12] = {"textured crop, stubble", "shoulder length, leather", "buzz cut, short beard", "bob, round frames", "undercut, full beard",
		"braids, flannel", "curls, goatee, shades", "ponytail, beanie", "side part, square frames", "afro, bomber", "shaved, ball cap", "crew cut, cap backwards"};
	for (int Page = 0; Page < 2; ++Page)
	{
		TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, Page == 0 ? "Character portraits" : "Character portraits, continued", "Live vector portraits from the creator's look: every hairstyle, face, jacket, hat and pair of glasses");
		for (int I = 0; I < 6; ++I)
		{
			const Pick& P = Cast[Page * 6 + I];
			hero::Character Who;
			Who.FirstName = P.First;
			Who.Age = P.Age;
			Who.Story = P.Story;
			Who.Appearance = P.L;
			const float X = 48.0f + static_cast<float>(I % 3) * 512.0f;
			const float Y = 124.0f + static_cast<float>(I / 3) * 432.0f;
			const ss::ui::Rect R{X, Y, 488.0f, 412.0f};
			C.FillRoundRect(R, 14.0f, ss::ui::Paint::Radial({X + 200.0f, Y + 120.0f}, 0.0f, {X + 244.0f, Y + 206.0f}, 330.0f, Hex(0x24304a), 0.5f, Hex(0x141b2c), Hex(0x0a0e18)));
			C.PushClip(R);
			ss::ui::DrawPortrait(C, Who, X + 244.0f, Y + 170.0f, 1.0f, 0.0);
			C.PopClip();
			C.FillRoundRect({X, Y + R.H - 54.0f, R.W, 54.0f}, 0.0f, ss::ui::Rgba(4, 6, 12, 0.82f));
			C.Text(std::string(P.First) + ", " + std::to_string(P.Age), X + 20.0f, Y + R.H - 22.0f, ss::ui::Ts(20.0f, 800, Hex(0xffffff)));
			C.Text(Labels[Page * 6 + I], X + R.W - 20.0f, Y + R.H - 22.0f, ss::ui::Ts(14.0f, 600, Hex(0x8b9bb4), ss::ui::Align::Right));
		}
		SaveSheet(Page == 0 ? "creator_portraits" : "creator_portraits_2", L);
	}
}

/** Bracelets and rings as their winners keep them, and the frames champions wear at the tables. */
void TrophyGallery()
{
	namespace ea = ss::ui::eventart;
	TableMeasurer M;
	const double Time = 2.4;
	auto Make = [](bool Ring, bool Online, bool Main, const std::string& Series, const std::string& Event) {
		ss::world::Award A;
		A.Ring = Ring;
		A.Online = Online;
		A.Main = Main;
		A.Series = Series;
		A.Event = Event;
		return A;
	};
	const std::vector<std::pair<ss::world::Award, std::string>> Shelf = {
		{Make(false, true, false, "tco27", "TCO '27 #12: $215 Final Viper"), "Championship Online bracelet"},
		{Make(false, true, true, "tco27", "TCO '27 #56: $5,300 Online Championship"), "Online Championship (Main)"},
		{Make(false, false, false, "The Championship 2027", "The Championship 2027: $1,500 Bounty"), "Las Vegas bracelet"},
		{Make(false, false, true, "The Championship 2027", "The Championship 2027: $10,000 Main Event"), "Championship Main Event"},
		{Make(true, true, false, "ring27", "RING '27 #3: $109 Iron Renegade"), "Ring Rush ring"},
		{Make(true, true, true, "ring27", "RING '27 #54: $1,050 Ring Main Event"), "Ring Main Event"},
		{Make(true, false, false, "Grand Circuit Montreal 2027", "Grand Circuit Montreal 2027: $580 Opener"), "Grand Circuit Montreal"},
		{Make(true, false, false, "Grand Circuit Prague 2027", "Grand Circuit Prague 2027: $1,100 Bounty"), "Grand Circuit Prague"},
		{Make(true, false, false, "Grand Circuit Sydney 2027", "Grand Circuit Sydney 2027: $5,300 Championship"), "Grand Circuit Sydney"},
		{Make(true, false, true, "Grand Circuit Montreal 2027", "Grand Circuit Montreal 2027: $1,700 Main Event"), "Grand Circuit Main Event"},
	};
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "AWARDS \xC2\xB7 BRACELETS AND RINGS", "Each series' own design; a Main Event's carries more");
		for (size_t I = 0; I < Shelf.size(); ++I)
		{
			const float X = 170.0f + static_cast<float>(I % 5) * 315.0f;
			const float Y = 280.0f + static_cast<float>(I / 5) * 360.0f;
			C.FillRoundRect({X - 130.0f, Y - 130.0f, 260.0f, 300.0f}, 22.0f, ss::ui::Hex(0x111c2e));
			ea::Trophy(C, Shelf[I].first, X, Y, 200.0f, Time + static_cast<double>(I));
			C.Text(Shelf[I].second, X, Y + 128.0f, ss::ui::Ts(15.0f, 800, ss::ui::Hex(0xffffff), ss::ui::Align::Center));
			C.Text(ea::TrophyEvent(Shelf[I].first), X, Y + 150.0f, ss::ui::Ts(11.0f, 600, ss::ui::Hex(0x8b9bb4), ss::ui::Align::Center, ss::ui::Baseline::Alphabetic, false, 240.0f));
			// And as a profile's shelf shows them.
			ea::Trophy(C, Shelf[I].first, X + 98.0f, Y - 102.0f, 40.0f, Time);
		}
		SaveSheet("awards_trophies", L);
	}
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		SheetBackground(C, "AWARDS \xC2\xB7 CHAMPIONS' FRAMES", "Bracelet winners wear gold links, ring winners their ring's stone; the player and the rival keep their glow");
		struct Case
		{
			std::string Name;
			std::vector<ss::world::Award> Won;
			bool Hero;
			std::string Label;
		};
		const std::vector<Case> Cases = {
			{"VelvetRiver", {Shelf[0].first}, false, "One bracelet (online)"},
			{"kenji.k", {Shelf[2].first, Shelf[3].first, Shelf[0].first}, false, "Three bracelets"},
			{"ElTiburon", {Shelf[4].first}, false, "A Ring Rush ring"},
			{"BramvdBerg", {Shelf[6].first, Shelf[7].first}, false, "Two circuit rings"},
			{"lazy_owl", {Shelf[1].first, Shelf[8].first}, false, "A bracelet and a ring"},
			{"grinder_3c", {Shelf[0].first}, true, "The player, a champion"},
		};
		const float Radii[3] = {58.0f, 21.0f, 11.5f};
		for (size_t I = 0; I < Cases.size(); ++I)
		{
			const float X = 150.0f + static_cast<float>(I) * 262.0f;
			ss::ui::AvatarSpec Pic = ss::ui::AvatarFor(Cases[I].Name);
			if (Cases[I].Hero)
			{
				Pic.Frame = ss::ui::AvatarFrame::Neon;
				Pic.Rim = 0x27d3c3;
			}
			ea::Champion(Pic, Cases[I].Won);
			Expect(Pic.Frame == ss::ui::AvatarFrame::Bracelet || Pic.Frame == ss::ui::AvatarFrame::Gem, "a champion's frame");
			float Y = 300.0f;
			for (float R : Radii)
			{
				ss::ui::DrawAvatar(C, X, Y, R, Pic);
				Y += R * 2.0f + 70.0f;
			}
			C.Text(Cases[I].Label, X, 720.0f, ss::ui::Ts(15.0f, 800, ss::ui::Hex(0xffffff), ss::ui::Align::Center));
			C.Text(Cases[I].Name, X, 742.0f, ss::ui::Ts(12.0f, 600, ss::ui::Hex(0x8b9bb4), ss::ui::Align::Center));
		}
		SaveSheet("awards_frames", L);
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
	ui_test::TrophyGallery();
	ui_test::PortraitGallery();
	ui_test::StoreScreens();
	ui_test::StreetProps();
	ui_test::MultiScreens();
	ui_test::StreamGallery();
	ui_test::StreamScreens();
	ui_test::PennyDropScreens();
	ui_test::LedScreens();
	ui_test::FrontEndFlows();
	ui_test::FrontEndScreens();
	if (ui_test::Failures == 0)
	{
		std::printf("ui tests: all passed\n");
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
