// riverline_ui: ui-front's additions (update 2 SPEC §3.8, §6.7): FreshCreator (THE WORLD step: the stage order,
// Established by default, NewCareerIn receiving Blank, Back and Escape keeping the choice) and the menu_creator_world dump.
// Compiled into ui_test and called from main() in ui_main.cpp (the kit is UiSeg.h). Phase A, day 1 (the lead): the entry
// point, empty; ui-front fills it, its helpers in ui_test::creator_seg.
//
// Also here: the FRESH START pill on the review's player card and on the CONTINUE card (DescribeSession's Blank), the
// GAMEPLAY tab's autoplay speed row and its settings key (autospeed, §6.4; a file without it reads 1x, §6.5), every
// settings label clearing its control down to 5:4, and the copy lint over THE WORLD's words and the row's help.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace creator_detail
{
// FrontEndCreator.cpp: THE WORLD's words, keyed (the cards, the board, the line under them, the pill).
std::vector<std::pair<std::string, std::string>> CreatorWorldInventory();
} // namespace creator_detail
namespace menu_detail
{
// FrontEnd.cpp: the autoplay speed row's help, for a pad or the keys.
std::string MenuAutoSpeedHelp(bool Gamepad);
} // namespace menu_detail
} // namespace ui
} // namespace ss

namespace ui_test
{
namespace creator_seg
{
using ss::ui::DrawList;
using ss::ui::FrontEnd;
using Stage = ss::ui::FrontEnd::CreatorStage;
using Start = ss::world::Start;

/** Every new career the menu starts, and how it reached the host. */
struct CreatorHooks : ss::ui::FrontEndHooks
{
	int Careers = 0;
	int OldCareers = 0;
	std::string Name;
	ss::hero::Character Who;
	Start World = Start::Established;
	int Changes = 0;
	void NewCareer(const std::string& ScreenName, const ss::hero::Character& Person) override
	{
		// A host's two-argument override: reached only when NewCareerIn isn't overridden.
		++OldCareers;
		Name = ScreenName;
		Who = Person;
	}
	void NewCareerIn(const std::string& ScreenName, const ss::hero::Character& Person, Start S) override
	{
		++Careers;
		Name = ScreenName;
		Who = Person;
		World = S;
	}
	void SettingsChanged(const ss::ui::GameSettings&) override { ++Changes; }
};

/** A host from before update 2: only the two-argument NewCareer. */
struct OldHooks : ss::ui::FrontEndHooks
{
	int Careers = 0;
	void NewCareer(const std::string&, const ss::hero::Character&) override { ++Careers; }
};

/** The portrait card's width and the form column's beside it (FrontEndShared.h's, from the outside). */
float CardWidth(float ViewW)
{
	return std::clamp(ViewW * 0.31f, 400.0f, 640.0f);
}

float FormWidth(float ViewW)
{
	return std::min(1060.0f, ViewW - 132.0f * 2.0f - CardWidth(ViewW) - 64.0f);
}

DrawList Frame(FrontEnd& Fe, double Now, float Width = 1920.0f)
{
	TableMeasurer M;
	DrawList L;
	ss::ui::Canvas C(L, M, Width, FrontEnd::Height, 1.0f);
	Fe.Draw(C, Now);
	return L;
}

/** Draws frames for a while, so the entrance animations settle. */
double Settle(FrontEnd& Fe, double Now, double Seconds, float Width = 1920.0f)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		Frame(Fe, Now, Width);
	}
	return Now;
}

/** A click at (X, Y): pressed on one frame, released on the next. */
double Click(FrontEnd& Fe, double Now, float X, float Y)
{
	Fe.Ptr.X = X;
	Fe.Ptr.Y = Y;
	Fe.Ptr.Active = true;
	Fe.Ptr.Down = true;
	Fe.Ptr.Pressed = true;
	Now += 1.0 / 30.0;
	Frame(Fe, Now);
	Fe.Ptr.Down = false;
	Fe.Ptr.Released = true;
	Now += 1.0 / 30.0;
	Frame(Fe, Now);
	return Now;
}

/** Letter-spaced labels draw a glyph a run: the runs on one baseline, joined (as ui_seg_tableui's Joined). */
std::string Joined(const DrawList& L)
{
	TableMeasurer M;
	std::string Out;
	float Base = -1.0f;
	float End = 0.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const float Wd = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const bool Same = std::fabs(Cmd.Text.BaselineY - Base) < 0.5f && Cmd.Text.X - End < Cmd.Text.SizePx * 0.6f && Cmd.Text.X >= End - 1.0f;
		Out += Same ? "" : "\n";
		Out += Cmd.Text.Text;
		Base = Cmd.Text.BaselineY;
		End = Cmd.Text.X + Wd;
	}
	return Out;
}

bool Says(const DrawList& L, const std::string& Needle)
{
	return Drew(L, Needle) || Joined(L).find(Needle) != std::string::npos;
}

/** Where a letter-spaced word starts (its first glyph's run: x and baseline), or Found false. */
struct Spot
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
};

Spot GlyphsAt(const DrawList& L, const std::string& Word)
{
	std::vector<const ss::ui::TextItem*> Runs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Runs.push_back(&Cmd.Text);
		}
	}
	Spot Out;
	for (size_t I = 0; I + Word.size() <= Runs.size(); ++I)
	{
		bool Match = true;
		for (size_t K = 0; K < Word.size() && Match; ++K)
		{
			Match = Runs[I + K]->Text == std::string(1, Word[K]) && std::fabs(Runs[I + K]->BaselineY - Runs[I]->BaselineY) < 0.5f;
		}
		if (Match)
		{
			Out.Found = true;
			Out.X = Runs[I]->X;
			Out.Base = Runs[I]->BaselineY;
			return Out;
		}
	}
	return Out;
}

/** Nothing runs off the screen. */
void FitsScreen(const DrawList& L, float Width, const std::string& What)
{
	TableMeasurer M;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.empty())
		{
			continue;
		}
		const float Wd = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < -1.0f || Cmd.Text.X + Wd > Width + 1.0f || Cmd.Text.BaselineY > 1081.0f)
		{
			Expect(false, (What + ": text fits the screen (\"" + Cmd.Text.Text + "\" runs off)").c_str());
			return;
		}
	}
}

/**
 * The creator's form keeps to its column: every run that starts left of the portrait card, between the stepper and the
 * buttons, ends before the card does; and nothing runs off the screen.
 */
void FitsForm(const DrawList& L, float Width, const std::string& What)
{
	FitsScreen(L, Width, What);
	TableMeasurer M;
	const float CardLeft = Width - 132.0f - CardWidth(Width);
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.empty())
		{
			continue;
		}
		const float Wd = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < CardLeft && Cmd.Text.BaselineY > 300.0f && Cmd.Text.BaselineY < 900.0f && Cmd.Text.X + Wd > CardLeft - 4.0f)
		{
			Expect(false, (What + ": the form keeps to its column (\"" + Cmd.Text.Text + "\" runs under the portrait)").c_str());
			return;
		}
	}
}

/** Opens the creator from the main menu (no save: NEW GAME is selected) and walks it to Target with Enter. */
double Walk(FrontEnd& Fe, double Now, Stage Target)
{
	Fe.Open(FrontEnd::Page::Main, Now);
	Now = Settle(Fe, Now, 0.8);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 0.4);
	while (Fe.Current() == FrontEnd::Page::NewGame && Fe.CreatorStep() != Target)
	{
		const Stage Was = Fe.CreatorStep();
		Fe.Key("Enter", Now);
		Now = Settle(Fe, Now, 0.2);
		if (Fe.CreatorStep() == Was)
		{
			break;
		}
	}
	return Settle(Fe, Now, 0.8);
}

/** A draft with a background whose name can't be mistaken for the world's pill. */
void Plain(FrontEnd& Fe)
{
	Fe.Draft.FirstName = "Rosa";
	Fe.Draft.LastName = "Delgado";
	Fe.Draft.Country = "MX";
	Fe.Draft.Age = 31;
	Fe.Draft.Story = ss::hero::Background::Kitchen;
	Fe.ScreenName = "rosa_d";
}

// ------------------------------------------------------------------ FreshCreator

void StageOrder()
{
	Expect(static_cast<int>(Stage::Identity) == 0 && static_cast<int>(Stage::Background) == 1 && static_cast<int>(Stage::Look) == 2 && static_cast<int>(Stage::World) == 3 &&
			   static_cast<int>(Stage::Review) == 4,
		"FreshCreator: THE WORLD sits between the look and the review");
	CreatorHooks H;
	FrontEnd Fe(H);
	double Now = 0.0;
	Expect(Fe.WorldStart == Start::Established, "FreshCreator: the menu starts on an established world");
	Fe.Open(FrontEnd::Page::Main, Now);
	Now = Settle(Fe, Now, 0.8);
	Fe.Key("Enter", Now);
	Plain(Fe);
	Now = Settle(Fe, Now, 0.4);
	std::vector<Stage> Seen = {Fe.CreatorStep()};
	std::vector<std::string> Headers;
	for (int K = 0; K < 4; ++K)
	{
		Now = Settle(Fe, Now, 0.6);
		const DrawList L = Frame(Fe, Now);
		Headers.push_back(Joined(L));
		Fe.Key("Enter", Now);
		Seen.push_back(Fe.CreatorStep());
	}
	Expect(Seen == std::vector<Stage>({Stage::Identity, Stage::Background, Stage::Look, Stage::World, Stage::Review}), "FreshCreator: Enter walks identity, background, look, world, review");
	Expect(Headers.size() == 4 && Headers[0].find("STEP 1 OF 5") != std::string::npos && Headers[2].find("STEP 3 OF 5") != std::string::npos &&
			   Headers[3].find("STEP 4 OF 5") != std::string::npos && Headers[3].find("THE WORLD") != std::string::npos,
		"FreshCreator: the header counts five steps, and the fourth is THE WORLD");
	Now = Settle(Fe, Now, 0.6);
	{
		const DrawList L = Frame(Fe, Now);
		Expect(Says(L, "STEP 5 OF 5") && Says(L, "THIS IS YOU") && Says(L, "WORLD") && Says(L, "REVIEW"), "FreshCreator: the review is the fifth step, with WORLD in the stepper");
		Expect(!Says(L, "FRESH START"), "FreshCreator: an established world's card wears no FRESH START pill");
	}
	Fe.Key("Escape", Now);
	Expect(Fe.CreatorStep() == Stage::World, "FreshCreator: Escape on the review steps back to THE WORLD");
	Fe.Key("Escape", Now);
	Expect(Fe.CreatorStep() == Stage::Look, "FreshCreator: and from THE WORLD to the look");
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Expect(H.Careers == 1 && H.OldCareers == 0 && H.World == Start::Established && H.Name == "rosa_d" && H.Who.FirstName == "Rosa" && H.Who.Created && !Fe.IsOpen(),
		"FreshCreator: untouched, BEGIN starts the career in an established world, through NewCareerIn");
}

void ThePick()
{
	CreatorHooks H;
	FrontEnd Fe(H);
	double Now = Walk(Fe, 0.0, Stage::World);
	Expect(Fe.CreatorStep() == Stage::World && Fe.WorldStart == Start::Established, "FreshCreator: THE WORLD opens on Established");
	{
		const DrawList L = Frame(Fe, Now);
		Expect(Drew(L, "Established world") && Drew(L, "Fresh start") && Drew(L, "Nobody's scored yet.") && Says(L, "SINCE OCT 6") && Says(L, "SINCE JAN 1") && Says(L, "EITHER WAY"),
			"FreshCreator: both cards draw, with the boards they open on and the line under them");
		Expect(Drew(L, "RiverLine's been running for years.") && Drew(L, "Everyone's record starts where yours does:"), "FreshCreator: each card says what it means");
		Expect(Says(L, "CHOOSE") && Says(L, "CONTINUE") && !Says(L, "RANDOM"), "FreshCreator: the hints choose and continue; no dice on THE WORLD");
	}
	Fe.Key("Right", Now);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: Right picks the fresh start");
	Fe.Key("Right", Now);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: Right again stays on it");
	Fe.Key("Up", Now);
	Fe.Key("Down", Now);
	Expect(Fe.WorldStart == Start::Blank && Fe.CreatorStep() == Stage::World, "FreshCreator: Up and Down change nothing");
	Fe.Key("Left", Now);
	Expect(Fe.WorldStart == Start::Established, "FreshCreator: Left picks the established world");
	Fe.Key("Tab", Now);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: Tab flips the pick");
	Fe.Key("TabPrev", Now);
	Expect(Fe.WorldStart == Start::Established, "FreshCreator: so do the bumpers");
	Fe.Key("R", Now);
	Fe.Key("Reset", Now);
	Fe.Key("Backspace", Now);
	Fe.Key("Undo", Now);
	Expect(Fe.WorldStart == Start::Established && Fe.CreatorStep() == Stage::World, "FreshCreator: the dice, Backspace and undo don't touch THE WORLD");

	// The pointer: a click on either card picks it.
	const float W = FormWidth(1920.0f);
	const float Cw = (W - 24.0f) * 0.5f;
	Now = Click(Fe, Now, 132.0f + Cw + 24.0f + Cw * 0.5f, 340.0f + 300.0f);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: a click on the right card picks the fresh start");
	Now = Click(Fe, Now, 132.0f + Cw * 0.5f, 340.0f + 100.0f);
	Expect(Fe.WorldStart == Start::Established, "FreshCreator: a click on the left card picks the established world");
	Now = Click(Fe, Now, 132.0f + Cw + 24.0f + 40.0f, 340.0f + 20.0f);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: anywhere on the card");

	// Back and Escape keep the choice: to the look and in again, then out to the menu and in again.
	Fe.Key("Escape", Now);
	Expect(Fe.CreatorStep() == Stage::Look && Fe.WorldStart == Start::Blank, "FreshCreator: Escape keeps the pick");
	Fe.Key("Enter", Now);
	Expect(Fe.CreatorStep() == Stage::World && Fe.WorldStart == Start::Blank, "FreshCreator: and it's still picked coming back");
	Now = Settle(Fe, Now, 0.6);
	{
		// The BACK button too.
		const DrawList L = Frame(Fe, Now);
		(void)L;
		Now = Click(Fe, Now, 132.0f + 90.0f, 908.0f + 30.0f);
		Expect(Fe.CreatorStep() == Stage::Look && Fe.WorldStart == Start::Blank, "FreshCreator: BACK keeps the pick");
	}
	const std::string Person = Fe.Draft.FullName();
	for (int K = 0; K < 3; ++K)
	{
		Fe.Key("Escape", Now);
	}
	Expect(Fe.Current() == FrontEnd::Page::Main, "FreshCreator: Escape backs out to the menu");
	Now = Settle(Fe, Now, 0.6);
	Expect(Says(Frame(Fe, Now), "IN THE CREATOR") && Says(Frame(Fe, Now), "FRESH START"), "FreshCreator: the NEW GAME card remembers the draft and its world");
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == FrontEnd::Page::NewGame && Fe.Draft.FullName() == Person && Fe.WorldStart == Start::Blank, "FreshCreator: in again, the draft and its world are kept");
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Expect(Fe.CreatorStep() == Stage::World && Fe.WorldStart == Start::Blank, "FreshCreator: THE WORLD still has the fresh start picked");

	// The review wears the pill; BEGIN hands the host a fresh start.
	Fe.Draft.Story = ss::hero::Background::Kitchen;
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 1.0);
	{
		const DrawList L = Frame(Fe, Now);
		const Spot Pill = GlyphsAt(L, "FRESH START");
		const Spot Member = GlyphsAt(L, "NEW MEMBER");
		Expect(Fe.CreatorStep() == Stage::Review && Pill.Found && Member.Found, "FreshCreator: the review's player card wears FRESH START");
		Expect(Pill.Found && Member.Found && std::fabs(Pill.Base - Member.Base) < 0.5f && Pill.X < Member.X, "FreshCreator: at 1920 the pill sits beside NEW MEMBER");
		WriteList("menu_creator_review_fresh", L);
	}
	{
		// 16:10 narrows the card: the pill moves onto the photo's foot, under the header and clear of the fields.
		Now = Settle(Fe, Now, 0.1, 1728.0f);
		const DrawList L = Frame(Fe, Now, 1728.0f);
		const Spot Pill = GlyphsAt(L, "FRESH START");
		const Spot Member = GlyphsAt(L, "NEW MEMBER");
		Expect(Pill.Found && Member.Found && Pill.Base > Member.Base + 200.0f && Pill.X < 132.0f + 28.0f + 196.0f, "FreshCreator: on a narrower card the pill is stamped on the photo");
		FitsForm(L, 1728.0f, "FreshCreator review at 1728");
	}
	Fe.Key("Enter", Now);
	Expect(H.Careers == 1 && H.World == Start::Blank && H.Who.FullName() == Person && H.Who.Created && !Fe.IsOpen(), "FreshCreator: NewCareerIn receives Blank");

	// The draft is spent: the next career starts from an established world again.
	Now = Walk(Fe, Now + 1.0, Stage::World);
	Expect(Fe.CreatorStep() == Stage::World && Fe.WorldStart == Start::Established, "FreshCreator: the next new career opens on Established again");
}

void OverwriteAndOldHosts()
{
	// BEGIN over a saved career asks first; START OVER still carries the pick.
	CreatorHooks H;
	FrontEnd Fe(H);
	Fe.Info.HasSave = true;
	double Now = 0.0;
	Fe.Open(FrontEnd::Page::Main, Now);
	Now = Settle(Fe, Now, 0.6);
	Fe.Key("Down", Now); // CONTINUE is selected with a save
	Fe.Key("Enter", Now);
	Plain(Fe);
	for (int K = 0; K < 3; ++K)
	{
		Fe.Key("Enter", Now);
	}
	Fe.Key("Right", Now);
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Expect(H.Careers == 0 && Fe.IsOpen(), "FreshCreator: BEGIN over a career asks first");
	Fe.Key("Left", Now);
	Fe.Key("Enter", Now);
	Expect(H.Careers == 1 && H.World == Start::Blank, "FreshCreator: START OVER begins it in the fresh start picked");

	// A host from before update 2 overrides only NewCareer: the default NewCareerIn forwards to it, whatever the pick.
	OldHooks Old;
	FrontEnd Fo(Old);
	Now = Walk(Fo, Now, Stage::World);
	Fo.Key("Right", Now);
	Fo.Key("Enter", Now);
	Fo.Key("Enter", Now);
	Expect(Old.Careers == 1 && !Fo.IsOpen(), "FreshCreator: a host with only NewCareer still gets the career");
}

void Gamepad()
{
	CreatorHooks H;
	FrontEnd Fe(H);
	Fe.Gamepad = true;
	double Now = Walk(Fe, 0.0, Stage::World);
	const DrawList L = Frame(Fe, Now);
	Expect(Says(L, "CHOOSE") && Says(L, "BACK"), "FreshCreator: the pad's hints on THE WORLD");
	Fe.Key("TabNext", Now);
	Expect(Fe.WorldStart == Start::Blank, "FreshCreator: RB flips the pick");
	Fe.Key("Left", Now);
	Expect(Fe.WorldStart == Start::Established, "FreshCreator: the d-pad picks");
}

/** The menu_creator_world dumps: both picks at 1920, and every width keeps the form in its column. */
void Screens()
{
	CreatorHooks H;
	FrontEnd Fe(H);
	Fe.Info.HasSave = false;
	Fe.CreatorSalt = "menu_creator_world";
	double Now = 100.0;
	Fe.Open(FrontEnd::Page::Main, Now);
	Now = Settle(Fe, Now, 0.8);
	Fe.Key("Enter", Now);
	Plain(Fe);
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Fe.Key("Enter", Now);
	Now = Settle(Fe, Now, 1.0);
	{
		const DrawList L = Frame(Fe, Now);
		Expect(!L.Cmds.empty() && Drew(L, "Established world"), "menu_creator_world drew");
		std::printf("  %-22s %6zu vertices %4zu commands\n", "menu_creator_world", L.Vertices.size(), L.Cmds.size());
		WriteList("menu_creator_world", L);
	}
	const float Widths[] = {1920.0f, 2520.0f, 1728.0f, 1440.0f, 1350.0f};
	for (int Pick = 0; Pick < 2; ++Pick)
	{
		Fe.Key(Pick == 0 ? "Left" : "Right", Now);
		Now = Settle(Fe, Now, 0.5);
		for (const float Width : Widths)
		{
			const DrawList L = Frame(Fe, Now, Width);
			const std::string What = std::string("THE WORLD (") + (Pick == 0 ? "established" : "fresh") + ") at " + std::to_string(static_cast<int>(Width));
			Expect(Drew(L, "Established world") && Drew(L, "Fresh start") && Drew(L, "Nobody's scored yet."), (What + ": both cards").c_str());
			FitsForm(L, Width, What);
			if (Width == 1920.0f && Pick == 1)
			{
				WriteList("menu_creator_world_fresh", L);
			}
			if (Width == 1350.0f)
			{
				WriteList(Pick == 0 ? "menu_creator_world_1350" : "menu_creator_world_fresh_1350", L);
			}
		}
	}
	// The copy fits its cards at full size where there's room (no ellipsis at 1920), and at 5:4 it may only shrink.
	{
		const DrawList L = Frame(Fe, Now, 1920.0f);
		Expect(Joined(L).find("\xE2\x80\xA6") == std::string::npos, "THE WORLD at 1920: nothing is cut short");
	}
}

// ------------------------------------------------------------------ the CONTINUE card

void ContinueCard()
{
	QuietHooks Q;
	ss::hero::Character Who;
	Who.Created = true;
	Who.FirstName = "Rosa";
	Who.LastName = "Delgado";
	Who.Country = "MX";
	Who.Story = ss::hero::Background::DealersKid;
	ss::Session Fresh(Q, "ui-front-fresh");
	Fresh.NewCareer(Who, Start::Blank);
	ss::Session Old(Q, "ui-front-established");
	Old.NewCareer(Who);
	const ss::ui::FrontEndInfo Fi = ss::ui::DescribeSession(Fresh, true);
	const ss::ui::FrontEndInfo Oi = ss::ui::DescribeSession(Old, true);
	Expect(Fi.Blank && !Oi.Blank, "CONTINUE: DescribeSession says which careers began in a fresh start");

	auto Card = [&](const ss::ui::FrontEndInfo& Info, const char* Name) {
		CreatorHooks H;
		FrontEnd Fe(H);
		Fe.Info = Info;
		double Now = 10.0;
		Fe.Open(FrontEnd::Page::Main, Now);
		Now = Settle(Fe, Now, 1.0);
		const DrawList L = Frame(Fe, Now);
		if (Name)
		{
			WriteList(Name, L);
		}
		return L;
	};
	{
		const DrawList L = Card(Fi, "menu_main_continue_fresh");
		const Spot Pill = GlyphsAt(L, "FRESH START");
		const Spot Story = GlyphsAt(L, "DEALER'S KID");
		Expect(Pill.Found && Story.Found && std::fabs(Pill.Base - Story.Base) < 0.5f && Pill.X > Story.X, "CONTINUE: a fresh start's pill rides beside the background's");
		Expect(!Says(L, "WORLD"), "CONTINUE: with the pill shown, no WORLD row");
	}
	{
		const DrawList L = Card(Oi, nullptr);
		Expect(!Says(L, "FRESH START"), "CONTINUE: an established career wears no pill");
	}
	{
		// A career from before the creator has no person to wear it: the world gets a row of its own.
		ss::ui::FrontEndInfo Legacy = Fi;
		Legacy.Person.Created = false;
		const DrawList L = Card(Legacy, nullptr);
		Expect(Says(L, "WORLD") && Drew(L, "FRESH START"), "CONTINUE: without a person card the world is a row");
	}
	{
		// Every background: the pill beside the background's when it fits, else the world's row; never neither, never both
		// (today every name leaves room, CASINO BOUNCER the least).
		bool Each = true;
		for (int B = 0; B < ss::hero::BackgroundCount; ++B)
		{
			ss::ui::FrontEndInfo One = Fi;
			One.Person.Story = static_cast<ss::hero::Background>(B);
			const DrawList L = Card(One, nullptr);
			std::string Name = ss::hero::InfoOf(One.Person.Story).Name;
			std::transform(Name.begin(), Name.end(), Name.begin(), [](unsigned char Ch) { return static_cast<char>(std::toupper(Ch)); });
			const Spot Story = GlyphsAt(L, Name);
			const Spot Pill = GlyphsAt(L, "FRESH START");
			const bool Beside = Story.Found && Pill.Found && std::fabs(Pill.Base - Story.Base) < 0.5f && Pill.X > Story.X;
			Each = Each && Story.Found && Beside != Says(L, "WORLD");
		}
		Expect(Each, "CONTINUE: for every background, the pill beside it or the world's row; never neither, never both");
	}
}

// ------------------------------------------------------------------ autoplay's speed setting

void AutoSpeedSetting()
{
	ss::ui::GameSettings Old;
	Expect(ss::ui::GameSettings::Parse("shortstack.settings.v1\nquality=2\ntablepace=1\nhudscale=110\n", Old) && Old.AutoSpeed == 1 && Old.HudScale == 110,
		"autospeed: a settings file from before update 2 reads 1x");
	const std::pair<const char*, int> Cases[] = {{"1", 1}, {"2", 2}, {"3", 2}, {"4", 4}, {"7", 4}, {"8", 8}, {"64", 8}, {"0", 1}, {"-4", 1}, {"fast", 1}};
	bool Clamped = true;
	for (const auto& Case : Cases)
	{
		ss::ui::GameSettings S;
		Clamped = Clamped && ss::ui::GameSettings::Parse(std::string("shortstack.settings.v1\nautospeed=") + Case.first + "\n", S) && S.AutoSpeed == Case.second;
	}
	Expect(Clamped, "autospeed: Parse lands every value on 1x, 2x, 4x or 8x");
	ss::ui::GameSettings Fast;
	Fast.AutoSpeed = 8;
	const std::string Text = Fast.Serialize();
	ss::ui::GameSettings Back;
	Expect(Text.find("\nautospeed=8\n") != std::string::npos && ss::ui::GameSettings::Parse(Text, Back) && Back.AutoSpeed == 8 && Back.Serialize() == Text,
		"autospeed: saved and read back");

	// The GAMEPLAY tab's last row.
	CreatorHooks H;
	FrontEnd Fe(H);
	double Now = 0.0;
	Fe.Open(FrontEnd::Page::Main, Now);
	Now = Settle(Fe, Now, 0.6);
	Fe.Key("Down", Now); // SETTINGS (no save: NEW GAME was selected)
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == FrontEnd::Page::Settings, "autospeed: settings open from the menu");
	for (int K = 0; K < 4; ++K)
	{
		Fe.Key("E", Now);
	}
	for (int K = 0; K < 6; ++K)
	{
		Fe.Key("Down", Now);
	}
	Now = Settle(Fe, Now, 0.6);
	{
		const DrawList L = Frame(Fe, Now);
		Expect(Drew(L, "Autoplay watch speed") && Says(L, "1x") && Drew(L, "How fast you watch when you hand your seat"), "autospeed: the GAMEPLAY tab's last row, with its help");
		FitsScreen(L, 1920.0f, "settings GAMEPLAY");
		WriteList("settings_gameplay_autospeed", L);
	}
	const int Before = H.Changes;
	Fe.Key("Right", Now);
	const int Two = Fe.Settings.AutoSpeed;
	Fe.Key("Right", Now);
	const int Four = Fe.Settings.AutoSpeed;
	Fe.Key("Right", Now);
	Fe.Key("Right", Now);
	const int Eight = Fe.Settings.AutoSpeed;
	Fe.Key("Enter", Now); // wraps
	const int Wrapped = Fe.Settings.AutoSpeed;
	Fe.Key("Left", Now);
	Expect(Two == 2 && Four == 4 && Eight == 8 && Wrapped == 1 && Fe.Settings.AutoSpeed == 1 && H.Changes == Before + 4, "autospeed: the row steps 1x, 2x, 4x, 8x, and Enter wraps");
	Fe.Key("Right", Now);
	Fe.Key("Right", Now);
	Expect(Fe.Settings.AutoSpeed == 4 && Fe.Settings.TablePace == 0 && Fe.Settings.HudScale == 100, "autospeed: the row changes nothing else");
	Fe.Key("R", Now);
	Expect(Fe.Settings.AutoSpeed == 1, "autospeed: DEFAULTS puts it back to 1x");
	{
		// Every width, both devices: the help names the right control.
		Fe.Gamepad = true;
		Now = Settle(Fe, Now, 0.4);
		const DrawList Pad = Frame(Fe, Now, 1440.0f);
		Expect(Says(Pad, "d-pad") && !Says(Pad, "T changes it"), "autospeed: on a pad the help names the d-pad");
		FitsScreen(Pad, 1440.0f, "settings GAMEPLAY at 1440");
		Fe.Gamepad = false;
		const DrawList Keys = Frame(Fe, Now, 2520.0f);
		Expect(Says(Keys, "T changes it"), "autospeed: on the keys, T");
		FitsScreen(Keys, 2520.0f, "settings GAMEPLAY at 2520");
	}
}

/**
 * Every settings row's label clears its control (the left chevron, or a slider's track) on every tab, at 5:4, 4:3 and
 * 16:9: at 5:4 the longest labels ("Autoplay watch speed", "Chromatic aberration") shrink rather than run under it.
 */
void SettingsLabelsFit()
{
	TableMeasurer M;
	for (const float Width : {1350.0f, 1440.0f, 1920.0f})
	{
		CreatorHooks H;
		FrontEnd Fe(H);
		double Now = 0.0;
		Fe.Open(FrontEnd::Page::Main, Now);
		Now = Settle(Fe, Now, 0.6, Width);
		Fe.Key("Down", Now); // SETTINGS (no save: NEW GAME was selected)
		Fe.Key("Enter", Now);
		// The controls' right edge: a slider's value ends there; a choice's left chevron sits 314 in, a slider's track 330.
		const float Rx = 132.0f + std::min(900.0f, Width - 2.0f * 132.0f - 560.0f);
		int Labels = 0;
		std::string Over;
		for (int Tab = 0; Tab < 5; ++Tab)
		{
			Now = Settle(Fe, Now, 0.6, Width);
			const DrawList L = Frame(Fe, Now, Width);
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				// A row's label starts 8 past the margin, below the tabs.
				if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || std::fabs(Cmd.Text.X - 140.0f) > 0.5f || Cmd.Text.BaselineY < 340.0f || Cmd.Text.BaselineY > 900.0f)
				{
					continue;
				}
				bool Slider = false;
				for (const ss::ui::DrawCmd& Value : L.Cmds)
				{
					Slider = Slider || (Value.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Value.Text.BaselineY - Cmd.Text.BaselineY) < 0.5f &&
										   std::fabs(Value.Text.X + M.Width(Value.Text.Text, Value.Text.Face, Value.Text.SizePx) - Rx) < 1.5f);
				}
				++Labels;
				if (Cmd.Text.X + M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx) > Rx - (Slider ? 330.0f : 314.0f) - 4.0f)
				{
					Over += " \"" + Cmd.Text.Text + "\"";
				}
			}
			Fe.Key("E", Now);
		}
		Expect(Labels >= 30 && Over.empty(), ("settings at " + std::to_string(static_cast<int>(Width)) + ": every label clears its control" + Over).c_str());
	}
}

// ------------------------------------------------------------------ copy

void Copy()
{
	const std::vector<std::pair<std::string, std::string>> World = ss::ui::creator_detail::CreatorWorldInventory();
	std::vector<std::pair<std::string, std::string>> Sentences;
	int Problems = 0;
	for (const auto& Entry : World)
	{
		const bool Label = Entry.first.find(".name") != std::string::npos || Entry.first.find(".label") != std::string::npos || Entry.first.find("since") != std::string::npos ||
			Entry.first == "world.pill" || Entry.first == "world.step" || Entry.first == "world.title";
		if (!Label)
		{
			Sentences.push_back(Entry);
			continue;
		}
		// Labels: no sentence ending to lint, but the rest holds (no doubled spaces, nothing banned, never empty).
		const std::string Why = copylint::Check(Entry.second + ".", copylint::Pa, copylint::Samples());
		if (!Why.empty())
		{
			++Problems;
			std::printf("COPY (creator) %s: \"%s\": %s\n", Entry.first.c_str(), Entry.second.c_str(), Why.c_str());
		}
	}
	Sentences.push_back({"settings.autospeed.pad", ss::ui::menu_detail::MenuAutoSpeedHelp(true)});
	Sentences.push_back({"settings.autospeed.keys", ss::ui::menu_detail::MenuAutoSpeedHelp(false)});
	Problems += copylint::Run("creator", Sentences, copylint::LetterBody);
	Expect(World.size() >= 12 && Sentences.size() >= 6, "copy: THE WORLD's inventory is all there");
	Expect(Problems == 0, "copy: THE WORLD and the autoplay row pass the copy lint");
	// U1: the creator's world words never mention autoplay (they're about the world), and no real brand anywhere.
	bool Clean = true;
	for (const auto& Entry : World)
	{
		const std::string Low = copylint::Lower(" " + Entry.second + " ");
		for (const char* Word : {" ai ", " bot ", "autoplay", "youtube", "twitch", "pokerstars"})
		{
			Clean = Clean && Low.find(Word) == std::string::npos;
		}
	}
	Expect(Clean, "copy: THE WORLD names no autoplay and no real brand");
	// The settings row is system UI: it may say "AI autoplay" (U1), and does.
	Expect(ss::ui::menu_detail::MenuAutoSpeedHelp(false).find("AI autoplay") != std::string::npos, "copy: the settings row says AI autoplay, in the system's words");
}
} // namespace creator_seg

void SegCreator()
{
	creator_seg::StageOrder();
	creator_seg::ThePick();
	creator_seg::OverwriteAndOldHosts();
	creator_seg::Gamepad();
	creator_seg::Screens();
	creator_seg::ContinueCard();
	creator_seg::AutoSpeedSetting();
	creator_seg::SettingsLabelsFit();
	creator_seg::Copy();
}
} // namespace ui_test
