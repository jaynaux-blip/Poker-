// Northside's shop screens (the realism update's spec, §7.8's shops_ui_test): every page opens and draws (the draw lists
// written as goldens for web/scripts/render-drawlists.mjs), the key flows (arrows, Enter, Tab, Escape, Q/E, PutBack,
// OpenGrace), the HostRequests each page emits, text that fits (TestFontMetrics, Roboto's real widths) at 16:9, 4:3 and
// 2550x1220, the pawn's SELL / ticket / TICKETS / BUY paths, the talk card, the street HUD's additions, ShopArt's faces,
// and the creator and the portrait with Outfit 6 "None".
// Usage: shops_ui_test [out-dir]   (with no argument it only checks)
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/ShopArt.h"
#include "ShortStack/UI/ShopScreen.h"
#include "ShortStack/UI/StoreCounter.h"
#include "ShortStack/UI/TalkCard.h"
#include "ShortStack/UI/Captions.h"
#include "TestFontMetrics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace shops_ui
{
int Failures = 0;
int Checks = 0;
std::string OutDir;

void Expect(bool Condition, const std::string& What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

/** Text widths from the Chromium metrics table (Roboto, the family Unreal's UI font uses), as ui_test measures. */
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
			if (C0 >= 0xF0)
			{
				Len = 4;
				Cp = C0 & 0x07u;
			}
			else if (C0 >= 0xE0)
			{
				Len = 3;
				Cp = C0 & 0x0Fu;
			}
			else if (C0 >= 0xC0)
			{
				Len = 2;
				Cp = C0 & 0x1Fu;
			}
			for (size_t K = 1; K < Len && I + K < Text.size(); ++K)
			{
				Cp = (Cp << 6) | (static_cast<unsigned char>(Text[I + K]) & 0x3Fu);
			}
			I += Len;
			float Wd = 55.0f;
			if (Cp >= 32 && Cp < 127)
			{
				Wd = Ascii(Face)[Cp - 32];
			}
			else if (Cp == 0xB7)
			{
				Wd = Special(Face)[0];
			}
			else if (Cp == 0x2014)
			{
				Wd = Special(Face)[1];
			}
			else if (Cp == 0x2013)
			{
				Wd = Special(Face)[2];
			}
			else if (Cp == 0x2026)
			{
				Wd = Special(Face)[3];
			}
			else if (Cp == 0xE9)
			{
				Wd = Special(Face)[4];
			}
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

struct Hooks : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Inbox;
	void Text(const std::string& From, const std::string& Body) override { Inbox.emplace_back(From, Body); }
};

const TableMeasurer Measurer;

// The widths the screens are checked at: 16:9 (1920), the user's 2550x1220 (2257 at 1080 tall), and 4:3 (1440).
const float Wide = 1080.0f * 2550.0f / 1220.0f;
const float Widths[3] = {1920.0f, Wide, 1440.0f};

bool Drew(const ss::ui::DrawList& L, const std::string& Needle)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Needle) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** A letter-spaced label draws a glyph a run: the runs on one baseline, joined (to find "PAY $12.00" in tracked text). */
std::string Joined(const ss::ui::DrawList& L)
{
	std::string Out;
	float Base = -1.0f;
	float End = 0.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const float W = Measurer.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const bool Same = std::fabs(Cmd.Text.BaselineY - Base) < 0.5f && Cmd.Text.X - End < Cmd.Text.SizePx * 0.6f && Cmd.Text.X >= End - 1.0f;
		Out += Same ? "" : "\n";
		Out += Cmd.Text.Text;
		Base = Cmd.Text.BaselineY;
		End = Cmd.Text.X + W;
	}
	return Out;
}

/** A word spelled glyph by glyph in consecutive runs, wherever they sit (a stamp's letters, turned on the paper). */
bool Spelled(const ss::ui::DrawList& L, const std::string& Word)
{
	std::vector<const std::string*> Runs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Runs.push_back(&Cmd.Text.Text);
		}
	}
	for (size_t I = 0; I < Runs.size(); ++I)
	{
		std::string Got;
		for (size_t K = I; K < Runs.size() && Got.size() < Word.size(); ++K)
		{
			Got += *Runs[K];
		}
		if (Got == Word)
		{
			return true;
		}
	}
	return false;
}

bool Says(const ss::ui::DrawList& L, const std::string& Needle)
{
	return Drew(L, Needle) || Joined(L).find(Needle) != std::string::npos || Spelled(L, Needle);
}

/**
 * Text that fits: every run starts and ends on the canvas, and none is cut short with an ellipsis unless it's one of
 * the long things that may be (a blurb, a headline's body, a paragraph). Returns the offending runs.
 */
std::vector<std::string> Overflows(const ss::ui::DrawList& L, float Width)
{
	std::vector<std::string> Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.empty())
		{
			continue;
		}
		const float W = Measurer.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < -1.0f || Cmd.Text.X + W > Width + 1.0f)
		{
			Out.push_back(Cmd.Text.Text);
		}
	}
	return Out;
}

void Fits(const ss::ui::DrawList& L, float Width, const std::string& What)
{
	const std::vector<std::string> Bad = Overflows(L, Width);
	Expect(Bad.empty(), What + ": text fits the screen" + (Bad.empty() ? std::string() : " (\"" + Bad.front() + "\" runs off)"));
}

void Write(const std::string& Name, const ss::ui::DrawList& L)
{
	if (OutDir.empty())
	{
		return;
	}
	if (FILE* F = std::fopen((OutDir + "/" + Name + ".json").c_str(), "wb"))
	{
		const std::string Json = L.ToJson();
		std::fwrite(Json.data(), 1, Json.size(), F);
		std::fclose(F);
	}
}

/** One frame of a screen at Width, over a stand-in room. */
ss::ui::DrawList Frame(ss::ui::ShopScreen& Screen, float Width, double Now)
{
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, Measurer, Width, ss::ui::ShopScreen::Height, 1.0f);
	C.FillRect({0.0f, 0.0f, Width, 1080.0f}, ss::ui::Paint::Linear({0.0f, 0.0f}, {Width, 1080.0f}, ss::ui::Hex(0x3a3028), ss::ui::Hex(0x12100e)));
	Screen.Draw(C, Now);
	return L;
}

/** A click: press and release at (X, Y) over two frames. */
void Click(ss::ui::ShopScreen& Screen, float Width, float X, float Y, double& Now)
{
	Screen.Ptr.Active = true;
	Screen.Ptr.X = X;
	Screen.Ptr.Y = Y;
	Screen.Ptr.Down = true;
	Screen.Ptr.Pressed = true;
	Frame(Screen, Width, Now);
	Now += 0.05;
	Screen.Ptr.Down = false;
	Screen.Ptr.Released = true;
	Frame(Screen, Width, Now);
	Now += 0.05;
	Screen.Ptr.Active = false;
	Screen.Ptr.X = Screen.Ptr.Y = -1.0f;
}

/** Where a text run containing Needle was drawn (its middle), or (-1, -1). */
ss::ui::Vec2 Where(const ss::ui::DrawList& L, const std::string& Needle)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Needle) != std::string::npos)
		{
			const float W = Measurer.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
			return {Cmd.Text.X + W * 0.5f, Cmd.Text.BaselineY - Cmd.Text.SizePx * 0.35f};
		}
	}
	return {-1.0f, -1.0f};
}

int Count(const std::vector<ss::ui::HostRequest>& R, ss::ui::HostRequest::Kind K, const std::string& Name = std::string())
{
	int N = 0;
	for (const ss::ui::HostRequest& Q : R)
	{
		N += Q.K == K && (Name.empty() || Q.Name == Name) ? 1 : 0;
	}
	return N;
}

const ss::ui::HostRequest* Last(const std::vector<ss::ui::HostRequest>& R, ss::ui::HostRequest::Kind K)
{
	for (auto It = R.rbegin(); It != R.rend(); ++It)
	{
		if (It->K == K)
		{
			return &*It;
		}
	}
	return nullptr;
}

ss::hero::Character Someone()
{
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Age = 31;
	C.Story = ss::hero::Background::Kitchen;
	C.Appearance.Hair = 3;       // Textured Crop
	C.Appearance.FacialHair = 1; // Stubble
	C.Appearance.Outfit = 1;     // the bomber
	C.Appearance.Hat = 2;
	return C;
}

/** Everything the 3D can show. */
ss::ui::HostCaps AllCaps()
{
	ss::ui::HostCaps Caps;
	Caps.HairStyles.assign(static_cast<size_t>(ss::hero::OptionCount(ss::hero::Slot::Hair)), true);
	Caps.FacialHair.assign(static_cast<size_t>(ss::hero::OptionCount(ss::hero::Slot::FacialHair)), true);
	Caps.Brows.assign(static_cast<size_t>(ss::hero::OptionCount(ss::hero::Slot::Brows)), true);
	Caps.GarmentKinds = ~0ull;
	Caps.GearKinds = ~0ull;
	Caps.Curios = 3u;
	Caps.Preview3D = true;
	return Caps;
}

/** Days into the world (0 Monday) and the hour: the session's clock (no tournament: the lobby's). */
double At(int Day, double Hour)
{
	return static_cast<double>(Day) * ss::net::MinutesPerDay + Hour * 60.0 - ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay);
}

/** A career in the middle of things: created, fed enough, money in the bank, at Day and Hour. */
void Career(ss::Session& S, int Day, double Hour, ss::Chips Bank = 50000)
{
	S.CurrentScreen = ss::Screen::Lobby;
	S.NewCareer(Someone());
	S.BankrollCents = Bank;
	S.LobbyMinutes = At(Day, Hour);
	S.Life.Hunger = 62.0;
	S.Life.Thirst = 55.0;
	S.Life.Energy = 64.0;
	// A month since the last cut and shave: the hair's grown enough for anything.
	S.Life.Groom.CutAt = S.WorldMinutes() - 30.0 * ss::net::MinutesPerDay;
	S.Life.Groom.ShaveAt = S.WorldMinutes() - 30.0 * ss::net::MinutesPerDay;
}

// ------------------------------------------------------------------ every page opens and draws

struct PageCase
{
	const char* Venue;
	std::vector<const char*> Pages;
};

void EveryPage()
{
	const std::vector<PageCase> Cases = {
		{"barber", {"barber-chair1", "BEARD", "COLOR", "BROWS"}},
		{"thrift", {"thrift-rack-tops", "thrift-rack-outer", "thrift-rack-pants", "thrift-shoes", "thrift-hats", "thrift-register", "thrift-donate", "thrift-booth"}},
		{"gym", {"gym-desk", "gym-treadmill1", "gym-bag", "gym-weights", "gym-cooler"}},
		{"diner", {"diner-stool3", "diner-booth2", "PAPER", "SIT"}},
		{"pawn", {"pawn-counter", "pawn-cage", "pawn-cases", "pawn-electronics", "pawn-guitars", "YOURS"}},
		{"wardrobe", {"apartment", "dee", "thrift-booth"}},
	};
	for (const PageCase& Case : Cases)
	{
		for (const char* PageId : Case.Pages)
		{
			for (const float W : Widths)
			{
				Hooks H;
				ss::Session S(H, std::string("shops-ui-") + Case.Venue);
				Career(S, 5, 11.5); // a Saturday morning: everything's open, the gym staffed
				S.Life.BackRoomNetCents = 2500; // Dee's chip is in the hero's pocket
				std::unique_ptr<ss::ui::ShopScreen> Screen = ss::ui::MakeShopScreen(Case.Venue, S);
				Expect(Screen != nullptr, std::string("a screen for ") + Case.Venue);
				if (!Screen)
				{
					continue;
				}
				Screen->SetCaps(AllCaps());
				double Now = 10.0;
				Screen->Open(Now, PageId);
				Expect(Screen->IsOpen(), std::string(Case.Venue) + " opens on " + PageId);
				ss::ui::DrawList L = Frame(*Screen, W, Now + 0.05);
				Now += 1.0;
				L = Frame(*Screen, W, Now);
				const std::string What = std::string(Case.Venue) + "/" + PageId + " at " + std::to_string(static_cast<int>(W));
				Expect(L.Cmds.size() > 20 && L.Vertices.size() > 200, What + ": draws");
				Fits(L, W, What);
				Expect(Says(L, "WALK AWAY") || Says(L, "BACK"), What + ": the footer says how to leave");
				if (W == Widths[0] || W == Widths[1])
				{
					Write("shop_" + std::string(Case.Venue) + "_" + PageId + (W == Widths[0] ? "" : "_wide"), L);
				}
			}
		}
	}
	Hooks H;
	ss::Session S(H, "shops-ui-none");
	Expect(ss::ui::MakeShopScreen("store", S) == nullptr && ss::ui::MakeShopScreen("washfold", S) == nullptr, "no shop screen for the Lucky Penny (it keeps StoreCounter) or the Wash & Fold");
}

// ------------------------------------------------------------------ the barber

void Barber()
{
	Hooks H;
	ss::Session S(H, "shops-ui-barber");
	Career(S, 1, 11.0); // Tuesday, 11 AM
	auto Screen = ss::ui::MakeShopScreen("barber", S);
	Screen->SetCaps(AllCaps());
	double Now = 10.0;
	Screen->Open(Now, "barber-chair1");
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::Camera, "barber-mirror") == 1, "barber: opens on the mirror's shot");
	const ss::ui::HostRequest* P = Last(R, ss::ui::HostRequest::PreviewLook);
	Expect(P != nullptr && P->Look.Hat == 0, "barber: the mirror shows the hero, hat off in the chair");
	// OpenGrace: the E that sat them down is still held.
	const ss::Chips Was = S.BankrollCents;
	Screen->Key("Enter", Now + 0.1);
	Screen->Key("Tab", Now + 0.15);
	Screen->Key("Tab", Now + 0.2);
	Expect(Screen->IsOpen() && S.BankrollCents == Was && Screen->TakeRequests().empty(), "barber: keys in the grace window do nothing");
	Now += 1.0;
	// The book: pick a crew cut (the arrows move, Enter picks), and the mirror tries each one on the way.
	const std::vector<ss::shop::Offer>& All = ss::shop::Offers("barber");
	std::vector<const ss::shop::Offer*> Cuts;
	for (const ss::shop::Offer& O : All)
	{
		if (O.Tab == "CUT")
		{
			Cuts.push_back(&O);
		}
	}
	Expect(!Cuts.empty(), "barber: the CUT tab has cuts");
	int Crew = -1;
	for (size_t I = 0; I < Cuts.size(); ++I)
	{
		Crew = Cuts[I]->LookValue == 2 ? static_cast<int>(I) : Crew;
	}
	Expect(Crew >= 0, "barber: a crew cut on the board");
	// The cursor starts on the hero's own style; walk it to the crew cut.
	int At_ = 0;
	for (size_t I = 0; I < Cuts.size(); ++I)
	{
		At_ = Cuts[I]->LookValue == S.Person.Appearance.Hair ? static_cast<int>(I) : At_;
	}
	while (At_ > Crew)
	{
		Screen->Key("Left", Now);
		--At_;
	}
	while (At_ < Crew)
	{
		Screen->Key("Right", Now);
		++At_;
	}
	R = Screen->TakeRequests();
	P = Last(R, ss::ui::HostRequest::PreviewLook);
	Expect(P && P->Look.Hair == 2, "barber: the mirror tries on the style in focus");
	Screen->Key("Enter", Now);
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "ON THE TICKET") && Says(L, "CREW CUT"), "barber: Enter puts it on the ticket");
	Expect(Says(L, "Crew Cut") && Says(L, ss::Money(Cuts[static_cast<size_t>(Crew)]->PriceCents)), "barber: the card and the ticket carry the price");
	// Q/E: the beard tab, a short beard. Together: $5 off.
	Screen->Key("E", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.5);
	Expect(Says(L, "BEARD") && Says(L, "Short Beard"), "barber: E turns to the BEARD tab");
	std::vector<const ss::shop::Offer*> Beards;
	for (const ss::shop::Offer& O : All)
	{
		if (O.Tab == "BEARD")
		{
			Beards.push_back(&O);
		}
	}
	int Short = -1;
	for (size_t I = 0; I < Beards.size(); ++I)
	{
		Short = Beards[I]->LookValue == 4 ? static_cast<int>(I) : Short;
	}
	// From the hero's own (stubble) to the short beard, by clicking its card.
	Now += 0.6;
	const ss::ui::Vec2 Card = Where(L, Beards[static_cast<size_t>(std::max(0, Short))]->Name);
	Expect(Card.X > 0.0f, "barber: the short beard's card is on screen");
	Click(*Screen, 1920.0f, Card.X, Card.Y, Now);
	L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "CUT & BEARD") && Says(L, "5.00"), "barber: a cut and a beard together take $5 off");
	// The tip: Tab to confirm, then the tip row (20% by default), Right to 25%.
	Screen->Key("Tab", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.1);
	Expect(Says(L, "TIP LONNIE") && Says(L, "TIP 20%"), "barber: CONFIRM brings up the tip, 20% by default");
	Screen->Key("Right", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.2);
	Expect(Says(L, "TIP 25%"), "barber: Right raises the tip");
	ss::Session::ShopOrder O;
	O.Venue = "barber";
	O.Offers = {Cuts[static_cast<size_t>(Crew)]->Id, Beards[static_cast<size_t>(std::max(0, Short))]->Id};
	O.TipPercent = 25;
	const ss::Chips Due = S.ShopTotal(O);
	Expect(Due > 0 && Says(L, "PAY " + ss::Money(Due)), "barber: the button charges what the Session will (" + ss::Money(Due) + ")");
	Expect(Says(L, ss::Money(Due)), "barber: the ticket's total is the Session's");
	Write("shop_barber_tip", L);
	Screen->TakeRequests();
	const ss::Chips Before = S.BankrollCents;
	Screen->Key("Tab", Now + 0.3);
	R = Screen->TakeRequests();
	const ss::ui::HostRequest* B = Last(R, ss::ui::HostRequest::BeginService);
	Expect(B != nullptr && B->Name == O.Offers[0] + "," + O.Offers[1], "barber: paying begins the service with its offer ids");
	Expect(Count(R, ss::ui::HostRequest::EndPreview) == 1, "barber: the preview ends before the chair's cinematic");
	Expect(!Screen->IsOpen() && Screen->TakeLeave(), "barber: the page closes itself as the service starts");
	Expect(Before - S.BankrollCents == Due && S.TimeSkip.Active, "barber: charged once, and the skip runs");

	// Walking away: the hero stands up, the mirror lets go.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-barber-leave");
		Career(S2, 1, 11.0);
		auto Sc = ss::ui::MakeShopScreen("barber", S2);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "CUT");
		Sc->TakeRequests();
		Sc->Key("Alt", T + 1.0);
		R = Sc->TakeRequests();
		Expect(Count(R, ss::ui::HostRequest::Camera, "barber-reveal") == 1, "barber: the second action swings to the back of the head");
		Sc->Key("Escape", T + 1.1);
		R = Sc->TakeRequests();
		Expect(!Sc->IsOpen() && Sc->TakeLeave() && !Sc->TakeLeave(), "barber: Escape walks away, once");
		Expect(Count(R, ss::ui::HostRequest::Stand) == 1 && Count(R, ss::ui::HostRequest::EndPreview) == 1, "barber: walking away stands the hero up and ends the preview");
	}
	// The length rule and what the 3D can't show.
	{
		Hooks H3;
		ss::Session S3(H3, "shops-ui-barber-rules");
		Career(S3, 1, 11.0);
		S3.Person.Appearance.Hair = 1; // a buzz cut, cut today in Lonnie's chair: nothing longer yet
		S3.Life.Groom.CutAt = S3.WorldMinutes();
		S3.Life.Groom.Cuts = 1;
		ss::ui::HostCaps Caps = AllCaps();
		Caps.HairStyles[11] = false; // no bob groom for this face
		Caps.HairStyles[0] = false;  // nor a shaved head: short enough to cut, but only on paper
		auto Sc = ss::ui::MakeShopScreen("barber", S3);
		Sc->SetCaps(Caps);
		double T = 10.0;
		Sc->Open(T, "barber-chair1");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "GIVE IT THREE WEEKS"), "barber: a style longer than the hair's grown reads 'Give it three weeks'");
		Expect(Says(L, "Last cut today"), "barber: the book says when the last cut was");
		// Through the book card by card: the bob reads PORTRAIT ONLY on the way, and Enter on it does nothing.
		bool Portrait = false;
		bool OnPaper = false;
		for (int K = 0; K < 20; ++K)
		{
			Sc->Key("Left", T);
		}
		for (int K = 0; K < 20; ++K)
		{
			Sc->Key(K == 0 ? "Up" : "Right", T);
			T += 0.4;
			L = Frame(*Sc, 1920.0f, T);
			if (Says(L, "PORTRAIT ONLY"))
			{
				if (!Portrait)
				{
					Write("shop_barber_rules", L);
				}
				Portrait = true;
			}
			if (Says(L, "ON PAPER ONLY"))
			{
				Sc->Key("Enter", T);
				L = Frame(*Sc, 1920.0f, T + 0.1);
				Expect(!Says(L, "ON THE TICKET"), "barber: a PORTRAIT ONLY style can't go on the ticket");
				OnPaper = true;
			}
		}
		Expect(Portrait && OnPaper, "barber: a style the 3D can't show reads PORTRAIT ONLY, and the mirror says it's on paper");
	}
	// DECLINED: the card won't cover it.
	{
		Hooks H4;
		ss::Session S4(H4, "shops-ui-barber-declined");
		Career(S4, 1, 11.0, 900);
		auto Sc = ss::ui::MakeShopScreen("barber", S4);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "COLOR");
		T += 1.0;
		Sc->Key("Right", T); // not the hero's own color: the next one
		Sc->Key("Enter", T);
		Sc->Key("Tab", T);
		Sc->Key("Tab", T);
		L = Frame(*Sc, 1920.0f, T + 0.3);
		Expect(Sc->IsOpen() && Says(L, "DECLINED") && S4.BankrollCents == 900 && !S4.TimeSkip.Active, "barber: a color the card can't cover is DECLINED, and nothing starts");
		Expect(Count(Sc->TakeRequests(), ss::ui::HostRequest::BeginService) == 0, "barber: no service on a declined card");
		Write("shop_barber_declined", L);
	}
}

// ------------------------------------------------------------------ the thrift

void Thrift()
{
	Hooks H;
	ss::Session S(H, "shops-ui-thrift");
	Career(S, 5, 14.0); // Saturday afternoon: yellow tags at half
	auto Screen = ss::ui::MakeShopScreen("thrift", S);
	Screen->SetCaps(AllCaps());
	const std::vector<std::string> Rack = ss::ui::shopart::ThriftRack(S, ~0ull);
	Expect(Rack.size() >= 24 && Rack.size() <= 30, "thrift: 24-30 pieces on this week's racks");
	// A jacket the hero doesn't own.
	std::string Jacket;
	for (const std::string& Id : Rack)
	{
		const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id);
		if (G && std::string(ss::ui::shopart::RackOf(*G)) == "outer" && S.Life.Closet.Owned.count(Id) == 0 && Jacket.empty())
		{
			Jacket = Id;
		}
	}
	Expect(!Jacket.empty(), "thrift: a jacket on the rack");
	double Now = 10.0;
	Screen->Open(Now, "thrift-rack-outer");
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::Camera, "thrift-rack") == 1, "thrift: the rack's shot");
	Now += 1.0;
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "JACKETS") && Says(L, ss::wardrobe::Find(Jacket)->Name), "thrift: the rack shows its jackets");
	// Saturday's yellow tags: half the Friday price on the tag, the old price struck out.
	for (const std::string& Id : Rack)
	{
		const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id);
		if (G && ss::wardrobe::TagOf(*G) == 1)
		{
			Expect(ss::wardrobe::ThriftPrice(*G, S.WorldMinutes()) < ss::wardrobe::ThriftPrice(*G, S.WorldMinutes() - 1440.0) && ss::ui::shopart::YellowTag(*G, S.WorldMinutes()),
				"thrift: a yellow tag rings up for less on a Saturday");
			break;
		}
	}
	// Focus the jacket (walk the rack to it) and try it on: the booth, the three-way mirror, the preview.
	int Index = 0;
	int K = 0;
	for (const std::string& Id : Rack)
	{
		const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id);
		if (G && std::string(ss::ui::shopart::RackOf(*G)) == "outer")
		{
			Index = Id == Jacket ? K : Index;
			++K;
		}
	}
	for (int I = 0; I < Index; ++I)
	{
		Screen->Key("Right", Now);
	}
	Screen->TakeRequests();
	Screen->Key("Alt", Now);
	R = Screen->TakeRequests();
	const ss::ui::HostRequest* P = Last(R, ss::ui::HostRequest::PreviewLook);
	Expect(P != nullptr && P->Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Outer)] == Jacket, "thrift: TRY ON dresses the hero in it");
	Expect(Count(R, ss::ui::HostRequest::Camera, "thrift-fitting") == 1, "thrift: TRY ON goes to the three-way mirror");
	L = Frame(*Screen, 1920.0f, Now + 0.5);
	Expect(Says(L, "IN THE BOOTH") && Says(L, "KEEP IT"), "thrift: the booth asks keep or put back");
	Write("shop_thrift_booth", L);
	Screen->Key("Q", Now + 0.6);
	Expect(Screen->IsOpen(), "thrift: the tabs hold in the booth");
	Screen->Key("Enter", Now + 0.7);
	R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::EndPreview) == 1 && Count(R, ss::ui::HostRequest::Camera, "thrift-rack") == 1, "thrift: keeping it leaves the booth for the rack");
	L = Frame(*Screen, 1920.0f, Now + 1.0);
	Expect(Says(L, "ON THE RECEIPT"), "thrift: kept, it's on the receipt");
	// PutBack takes it off; Enter adds it again.
	Screen->Key("PutBack", Now + 1.1);
	L = Frame(*Screen, 1920.0f, Now + 1.2);
	Expect(!Says(L, "ON THE RECEIPT"), "thrift: PutBack takes it off the receipt");
	Screen->Key("Enter", Now + 1.3);
	// To the register (Tab), "Wear it out" on by default, and pay (Tab).
	Screen->Key("Tab", Now + 1.4);
	L = Frame(*Screen, 1920.0f, Now + 1.5);
	ss::Session::ShopOrder O;
	O.Venue = "thrift";
	O.Garments = {Jacket};
	const ss::Chips Due = S.ShopTotal(O);
	Expect(Says(L, "WEAR IT OUT") && Says(L, "PAY " + ss::Money(Due)), "thrift: the register: wear it out, and the Session's total with tax");
	Expect(Says(L, "TAX 7.25%"), "thrift: garments are taxed");
	const ss::Chips Before = S.BankrollCents;
	Screen->TakeRequests();
	Screen->Key("Tab", Now + 1.6);
	R = Screen->TakeRequests();
	L = Frame(*Screen, 1920.0f, Now + 1.7);
	Expect(Before - S.BankrollCents == Due && S.Life.Closet.Owned.count(Jacket) == 1, "thrift: paid, it's in the closet");
	Expect(S.Life.Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Outer)] == Jacket && Count(R, ss::ui::HostRequest::Redress) == 1, "thrift: worn out the door (Redress)");
	Expect(Says(L, "PAID") && Screen->IsOpen(), "thrift: the receipt is stamped PAID");
	Write("shop_thrift_paid", L);
	// The donation bin: Q back to DONATE (it wraps), give a tee away.
	Screen->Key("Q", Now + 2.0);
	Screen->Key("Q", Now + 2.0);
	for (int I = 0; I < 6; ++I)
	{
		Screen->Key("E", Now + 2.0);
	}
	L = Frame(*Screen, 1920.0f, Now + 2.2);
	Expect(Says(L, "ST. AGNES OUTREACH") && Says(L, "DONATION RECEIPT"), "thrift: the DONATE tab prints a donation receipt");
	const size_t Owned = S.Life.Closet.Owned.size();
	// The first thing owned that isn't being worn (the shorts the creator's closet comes with, say).
	int Spare = 0;
	for (const std::string& Id : S.Life.Closet.Owned)
	{
		if (std::find(S.Life.Closet.Worn.begin(), S.Life.Closet.Worn.end(), Id) == S.Life.Closet.Worn.end())
		{
			break;
		}
		++Spare;
	}
	for (int I = 0; I < Spare; ++I)
	{
		Screen->Key("Right", Now + 2.25);
	}
	Screen->Key("Enter", Now + 2.3);
	Screen->Key("Tab", Now + 2.4);
	L = Frame(*Screen, 1920.0f, Now + 2.5);
	Expect(S.Life.Closet.Owned.size() + 1 == Owned && Says(L, "THANK YOU") && Says(L, "tax records"), "thrift: a donation leaves the closet, with Bea's receipt for your taxes");
	Write("shop_thrift_donated", L);
}

// ------------------------------------------------------------------ the gym

void Gym()
{
	Hooks H;
	ss::Session S(H, "shops-ui-gym");
	Career(S, 2, 17.0); // Wednesday, 5 PM: staffed
	auto Screen = ss::ui::MakeShopScreen("gym", S);
	Screen->SetCaps(AllCaps());
	double Now = 10.0;
	Screen->Open(Now, "gym-desk");
	Now += 1.0;
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "FITNESS") && Says(L, "WHAT IT DOES") && Says(L, "NO MEMBERSHIP"), "gym: the desk shows the fitness card");
	Expect(Says(L, "ROSA"), "gym: Rosa's at the desk while staffed");
	// A day pass: Enter on it, Tab pays (goods: ShopPay).
	int Pass = -1;
	int I = 0;
	for (const ss::shop::Offer& O : ss::shop::Offers("gym"))
	{
		if (O.Type == ss::shop::OfferKind::Membership || O.Type == ss::shop::OfferKind::DayPass || O.Type == ss::shop::OfferKind::Shake)
		{
			Pass = O.Type == ss::shop::OfferKind::DayPass ? I : Pass;
			++I;
		}
	}
	Expect(Pass >= 0, "gym: a day pass at the desk");
	for (int K = 0; K < Pass; ++K)
	{
		Screen->Key("Right", Now);
	}
	Screen->Key("Enter", Now);
	const ss::Chips Before = S.BankrollCents;
	Screen->Key("Tab", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.2);
	Expect(Before - S.BankrollCents == 1000 && Says(L, "PAID") && Screen->IsOpen(), "gym: the $10 day pass is paid at the desk");
	// The floor: the treadmill, its cost to the body, START.
	Screen->Key("E", Now + 0.3);
	L = Frame(*Screen, 1920.0f, Now + 0.5);
	Expect(Says(L, "WORKOUTS") && Says(L, "Treadmill") && Says(L, "WHAT IT TAKES") && Says(L, "Fitness"), "gym: the floor lists the workouts and what they take");
	Expect(!Says(L, "PAID"), "gym: the desk's PAID stamp stays on the desk's receipt (the floor's bill is a quote)");
	Write("shop_gym_floor", L);
	Screen->TakeRequests();
	Screen->Key("Tab", Now + 0.6);
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	const ss::ui::HostRequest* B = Last(R, ss::ui::HostRequest::BeginService);
	Expect(B && B->Name.find("gym/") == 0 && !Screen->IsOpen() && S.TimeSkip.Active, "gym: START begins the workout");
	// A machine opens the floor on its own workout; between shifts the desk's card says when Rosa's back.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-gym-machine");
		Career(S2, 2, 14.0); // 2 PM: between shifts
		S2.Gear["gym"] = S2.WorldMinutes() + 20.0 * ss::net::MinutesPerDay; // a member on the fob
		auto Sc = ss::ui::MakeShopScreen("gym", S2);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "gym-bag");
		R = Sc->TakeRequests();
		Expect(Count(R, ss::ui::HostRequest::Camera, "gym-floor") == 1, "gym: a machine opens on the floor's shot");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "Heavy bag") && Says(L, "BACK AT 4 PM"), "gym: the bag in focus; the desk's card says BACK AT 4 PM");
		Write("shop_gym_backat", L);
	}
	// At a machine with no pass while the desk's staffed: the day pass goes on the workout's order, one START.
	{
		Hooks H3;
		ss::Session S3(H3, "shops-ui-gym-walkin");
		Career(S3, 2, 17.0);
		auto Sc = ss::ui::MakeShopScreen("gym", S3);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "gym-treadmill1");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "WITH A PASS") && Says(L, "DAY PASS + START") && Says(L, "DAY PASS \xC2\xB7 TODAY"), "gym: no pass at a machine: the day pass rides on START");
		Write("shop_gym_walkin", L);
		const ss::Chips Was = S3.BankrollCents;
		Sc->TakeRequests();
		Sc->Key("Tab", T + 0.1);
		R = Sc->TakeRequests();
		Expect(Was - S3.BankrollCents == 1000 && Last(R, ss::ui::HostRequest::BeginService) && S3.TimeSkip.Active && !Sc->IsOpen(), "gym: the $10 pass and the treadmill in one go");
	}
	// Out of the desk's hours and not a member: the machines say so, and START stays dark.
	{
		Hooks H4;
		ss::Session S4(H4, "shops-ui-gym-shut");
		Career(S4, 2, 14.0); // 2 PM: between shifts
		auto Sc = ss::ui::MakeShopScreen("gym", S4);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "gym-weights");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		const ss::Chips Was = S4.BankrollCents;
		Sc->Key("Tab", T + 0.1);
		Expect(Says(L, "WITH A PASS") && !Says(L, "DAY PASS + START") && Was == S4.BankrollCents && !S4.TimeSkip.Active, "gym: no pass and no desk: START stays dark");
		Write("shop_gym_shut", L);
	}
}

// ------------------------------------------------------------------ Ruby's

void Diner()
{
	Hooks H;
	ss::Session S(H, "shops-ui-diner");
	Career(S, 3, 2.5); // Thursday, 2:30 AM: Marlene's shift
	auto Screen = ss::ui::MakeShopScreen("diner", S);
	Screen->SetCaps(AllCaps());
	double Now = 10.0;
	Screen->Open(Now, "diner-stool3");
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::Camera, "diner-counter") == 1, "diner: a stool's shot");
	Now += 1.0;
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "MARLENE") && Says(L, "GUEST CHECK") && Says(L, "STOOL 3"), "diner: Marlene's guest check, at stool 3");
	Expect(Says(L, "The Short Stack") && Says(L, "$6.99"), "diner: the menu at its prices");
	Expect(Says(L, "WHAT IT WOULD DO"), "diner: the vitals preview the dish in focus");
	// The Short Stack and a coffee.
	Screen->Key("Enter", Now);
	int Coffee = -1;
	int I = 0;
	for (const ss::shop::Offer& O : ss::shop::Offers("diner"))
	{
		if (O.Type == ss::shop::OfferKind::Meal || O.Type == ss::shop::OfferKind::Drink)
		{
			Coffee = O.Id == "diner/coffee" ? I : Coffee;
			++I;
		}
	}
	for (int K = 0; K < Coffee; ++K)
	{
		Screen->Key("Right", Now);
	}
	Screen->Key("Enter", Now);
	Screen->Key("Tab", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.2);
	Expect(Says(L, "TIP MARLENE") && Says(L, "TIP 18%"), "diner: the check, 18% by default");
	ss::Session::ShopOrder O;
	O.Venue = "diner";
	O.Offers = {"diner/short-stack", "diner/coffee"};
	O.TipPercent = 18;
	const ss::Chips Due = S.ShopTotal(O);
	Expect(Due > 0 && Says(L, "PAY " + ss::Money(Due)), "diner: the check's total is the Session's");
	Write("shop_diner_check", L);
	Screen->TakeRequests();
	Screen->Key("Tab", Now + 0.3);
	R = Screen->TakeRequests();
	const ss::ui::HostRequest* B = Last(R, ss::ui::HostRequest::BeginService);
	Expect(B && B->Name == "diner/short-stack,diner/coffee" && !Screen->IsOpen(), "diner: paying sits the order down");
	// The paper and sitting a while.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-diner-sit");
		Career(S2, 3, 2.5);
		auto Sc = ss::ui::MakeShopScreen("diner", S2);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "PAPER");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "THE NORTHSIDE COURIER") && Says(L, "LATE EDITION"), "diner: the paper");
		// The weather box: four spells from the street's own model, named in order, the first agreeing with the sky.
		Expect(Says(L, "THE WEATHER") && Says(L, "OVERNIGHT") && Says(L, "THIS MORNING") && Says(L, "THIS AFTERNOON") && Says(L, "TONIGHT"),
			"diner: the paper's weather, overnight to tonight at 2:30 AM");
		{
			const int64_t Slot = static_cast<int64_t>(std::floor(S2.WorldMinutes() / ss::weather::SlotMinutes));
			int Count[5] = {0, 0, 0, 0, 0};
			for (int K = 0; K < 12; ++K)
			{
				++Count[std::clamp(static_cast<int>(ss::weather::SlotSky(Slot + K, S2.WorldSeed())), 0, 4)];
			}
			int Most = 0;
			for (int K = 1; K < 5; ++K)
			{
				Most = Count[K] > Count[Most] ? K : Most;
			}
			static const char* const Words[5] = {"Clear", "Cloudy", "Drizzle", "Rain", "Storms"};
			Expect(Drew(L, Words[Most]), std::string("diner: the paper's overnight reads ") + Words[Most] + ", as the street will");
		}
		Sc->Key("E", T);
		Sc->Key("Right", T);
		Sc->Key("Right", T);
		L = Frame(*Sc, 1920.0f, T + 0.2);
		Expect(Says(L, "AFTER 4 HOURS") && Says(L, "SIT UNTIL"), "diner: sit a while: four hours, until when");
		Write("shop_diner_sit", L);
		Sc->TakeRequests();
		Sc->Key("Tab", T + 0.3);
		R = Sc->TakeRequests();
		B = Last(R, ss::ui::HostRequest::BeginService);
		Expect(B && B->Name == "wait" && S2.TimeSkip.Active && !Sc->IsOpen(), "diner: sitting a while is a wait (BeginService \"wait\")");
	}
}

// ------------------------------------------------------------------ the pawn

/** The first GearDrop item Augie takes, owned (bought outright). */
std::string GiveGear(ss::Session& S)
{
	for (const ss::gear::Item& I : ss::gear::Catalog())
	{
		if (!I.Monthly && ss::shop::Takes(I) && I.PriceCents >= 10000 && I.Requires.empty())
		{
			S.Gear[I.Id] = 0.0;
			return I.Id;
		}
	}
	return std::string();
}

void Pawn()
{
	Hooks H;
	ss::Session S(H, "shops-ui-pawn");
	Career(S, 1, 12.0); // Tuesday noon
	S.Life.BackRoomNetCents = 2500;
	const std::string Gear = GiveGear(S);
	Expect(!Gear.empty(), "pawn: a piece of gear he takes");
	auto Screen = ss::ui::MakeShopScreen("pawn", S);
	Screen->SetCaps(AllCaps());
	double Now = 10.0;
	Screen->Open(Now, "pawn-counter");
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::Camera, "pawn-counter") == 1, "pawn: the counter's shot");
	Now += 1.0;
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	const std::vector<ss::shop::Lot> Lots = S.PawnLots();
	Expect(Says(L, "SELL") && Says(L, "AUGIE"), "pawn: SELL at the counter, Augie behind it");
	// A refused lot: Dee's chip. Enter shows his line instead of offers.
	int Chip = -1;
	int Gearl = -1;
	int Keep = -1;
	for (size_t I = 0; I < Lots.size(); ++I)
	{
		Chip = !Lots[I].Refused.empty() ? static_cast<int>(I) : Chip;
		Gearl = Lots[I].Id == "gear:" + Gear ? static_cast<int>(I) : Gearl;
		Keep = Lots[I].Kind == ss::shop::LotKind::Keepsake ? static_cast<int>(I) : Keep;
	}
	Expect(Chip >= 0 && Gearl >= 0 && Keep >= 0, "pawn: the gear, the keepsake and Dee's chip are on the list");
	for (int I = 0; I < Chip; ++I)
	{
		Screen->Key("Right", Now);
	}
	Screen->Key("Enter", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.1);
	Expect(Says(L, "She'd have my license") && Says(L, "HE WON'T TAKE IT"), "pawn: Dee's chip is refused, in his words");
	Write("shop_pawn_refused", L);
	// The keepsake: the loupe's beat, his two offers, then SELL asks once more (it doesn't come back).
	for (int I = 0; I < 20; ++I)
	{
		Screen->Key("Left", Now);
	}
	for (int I = 0; I < Keep; ++I)
	{
		Screen->Key("Right", Now);
	}
	Screen->TakeRequests();
	Screen->Key("Enter", Now);
	L = Frame(*Screen, 1920.0f, Now + 0.5);
	Expect(Says(L, "ON THE GLASS") && Says(L, "UNDER THE LOUPE"), "pawn: on the glass, under the loupe");
	Screen->Key("Enter", Now + 0.6);
	L = Frame(*Screen, 1920.0f, Now + 0.7);
	Expect(!Says(L, "THIS ONE DOESN'T COME BACK"), "pawn: no offers before the loupe's beat is done");
	L = Frame(*Screen, 1920.0f, Now + 1.8);
	const ss::shop::Lot& Ks = Lots[static_cast<size_t>(Keep)];
	Expect(Says(L, "SELL " + std::string("$") + std::to_string(Ks.SellCents / 100)) && Says(L, "PAWN $" + std::to_string(Ks.LoanCents / 100)), "pawn: his two offers, SELL and PAWN");
	// His words come up with the offers and fade in over the bubble's 0.3 s.
	L = Frame(*Screen, 1920.0f, Now + 2.2);
	{
		const bool Numbers = Says(L, "to own it") || Says(L, "own it");
		if (!Numbers)
		{
			std::printf("  (the frame read:%s)\n", Joined(L).c_str());
		}
		Expect(Numbers, "pawn: Augie says the numbers");
	}
	Screen->Key("Enter", Now + 2.3);
	L = Frame(*Screen, 1920.0f, Now + 2.4);
	Expect(Says(L, "THIS ONE DOESN'T COME BACK") && Says(L, "KEEP IT"), "pawn: a Final lot asks again: SELL / KEEP");
	Write("shop_pawn_final_ask", L);
	Screen->Key("Enter", Now + 2.5); // KEEP is chosen: nothing sold
	Expect(S.PawnLots().size() == Lots.size(), "pawn: KEEP keeps it");
	// The gear: PAWN prints the pink ticket.
	for (int I = 0; I < 20; ++I)
	{
		Screen->Key("Left", Now + 2.6);
	}
	for (int I = 0; I < Gearl; ++I)
	{
		Screen->Key("Right", Now + 2.6);
	}
	Screen->Key("Enter", Now + 2.7);
	Frame(*Screen, 1920.0f, Now + 4.4);
	Screen->Key("Right", Now + 4.5); // PAWN
	const ss::Chips Before = S.BankrollCents;
	const ss::shop::Lot& Gl = Lots[static_cast<size_t>(Gearl)];
	Screen->Key("Enter", Now + 4.6);
	L = Frame(*Screen, 1920.0f, Now + 4.9);
	Expect(S.BankrollCents - Before == Gl.LoanCents && !S.Owns(Gear) && S.Life.Pawn.Tickets.size() == 1, "pawn: the loan through the ledger, the gear in his cage");
	const ss::shop::Ticket Tk = S.Life.Pawn.Tickets.front();
	Expect(Says(L, "PAWN TICKET") && Says(L, "#" + ss::shop::TicketNumber(Tk.Number)) && Says(L, "REDEEM BY"), "pawn: PAWN's receipt is the pawn ticket");
	Write("shop_pawn_ticket", L);
	Screen->Key("Escape", Now + 5.0);
	Expect(!Screen->IsOpen() && Screen->TakeLeave(), "pawn: walked away");
	// TICKETS at the cage: in term, then in grace (in red, +25%), and REDEEM.
	Screen->Open(Now + 6.0, "pawn-cage");
	R = Screen->TakeRequests();
	Expect(Count(R, ss::ui::HostRequest::Camera, "pawn-cage") == 1, "pawn: the cage's shot");
	L = Frame(*Screen, 1920.0f, Now + 7.0);
	Expect(Says(L, "DUE ") && Says(L, ss::Money(ss::shop::RedeemCents(Tk, S.WorldMinutes()))), "pawn: TICKETS shows the due date and today's price");
	Write("shop_pawn_tickets_term", L);
	S.LobbyMinutes += 33.0 * ss::net::MinutesPerDay;
	L = Frame(*Screen, 1920.0f, Now + 7.5);
	const ss::Chips Grace = ss::shop::RedeemCents(Tk, S.WorldMinutes());
	Expect(Says(L, "IN GRACE UNTIL") && Says(L, ss::Money(Grace)) && Grace > ss::shop::RedeemCents(Tk, Tk.DueAt - 1.0), "pawn: in grace, in red, and dearer than in term");
	// In the grace RENEW asks the grace's 25%, the same interest REDEEM charges: renewing is never the cheaper way out.
	const ss::Chips GraceFee = ss::shop::RenewCents(Tk, S.WorldMinutes());
	Expect(GraceFee == Grace - Tk.LoanCents && GraceFee == Tk.LoanCents / 4 && GraceFee > ss::shop::RenewCents(Tk) && Says(L, "RENEW 30 DAYS \xC2\xB7 " + ss::Money(GraceFee)),
		"pawn: in grace, RENEW is the 25% (" + ss::Money(GraceFee) + "), not the 20%");
	Write("shop_pawn_tickets_grace", L);
	S.BankrollCents = Grace + 10000;
	Screen->Key("Tab", Now + 8.0);
	L = Frame(*Screen, 1920.0f, Now + 8.2);
	Expect(S.Owns(Gear) && S.Life.Pawn.Tickets.empty() && Says(L, "REDEEMED"), "pawn: REDEEM brings it home");
	Expect(!Says(L, "RENEW"), "pawn: with no tickets left the footer offers only the way out");
	// BUY: the curio case with NOT IN STOCK, and the used gear with "Needs a room" while evicted.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-pawn-buy");
		Career(S2, 1, 12.0);
		ss::ui::HostCaps Caps = AllCaps();
		Caps.Curios = 0; // neither the chip nor the watch renders
		auto Sc = ss::ui::MakeShopScreen("pawn", S2);
		Sc->SetCaps(Caps);
		double T = 10.0;
		Sc->Open(T, "pawn-cases");
		R = Sc->TakeRequests();
		Expect(Count(R, ss::ui::HostRequest::Camera, "pawn-case") == 1, "pawn: the cases' shot");
		T += 1.0;
		L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "NOT IN STOCK"), "pawn: a curio the 3D can't show reads NOT IN STOCK");
		Write("shop_pawn_curios", L);
		// The aviators: buy them (taxed).
		const std::vector<ss::shop::Lot> Shelf = S2.PawnShelf();
		int Av = -1;
		int Ci = 0;
		for (const ss::shop::Lot& Lt : Shelf)
		{
			if (Lt.Kind == ss::shop::LotKind::Curio)
			{
				Av = Lt.Id == "curio:aviators" ? Ci : Av;
				++Ci;
			}
		}
		for (int K = 0; K < Av; ++K)
		{
			Sc->Key("Right", T);
		}
		Sc->Key("Enter", T);
		const ss::Chips Was = S2.BankrollCents;
		Sc->Key("Tab", T);
		L = Frame(*Sc, 1920.0f, T + 0.2);
		const ss::shop::Lot* Avi = nullptr;
		for (const ss::shop::Lot& Lt : Shelf)
		{
			Avi = Lt.Id == "curio:aviators" ? &Lt : Avi;
		}
		Expect(Avi && Was - S2.BankrollCents == Avi->ValueCents + ss::store::Tax(Avi->ValueCents) && Says(L, "PAID"), "pawn: the aviators bought, taxed");
		S2.Life.RentStage = ss::life::Rent::Evicted;
		Sc->Key("Q", T + 0.5);
		L = Frame(*Sc, 1920.0f, T + 1.0);
		Expect(Says(L, "GEAR") && Says(L, "NEEDS A ROOM"), "pawn: used gear while evicted reads 'Needs a room'");
		Write("shop_pawn_gear_evicted", L);
	}
}

// ------------------------------------------------------------------ the closet

void Wardrobe()
{
	Hooks H;
	ss::Session S(H, "shops-ui-wardrobe");
	Career(S, 1, 9.0);
	auto Screen = ss::ui::MakeShopScreen("wardrobe", S);
	ss::ui::HostCaps Caps = AllCaps();
	Caps.Preview3D = false;
	Screen->SetCaps(Caps);
	double Now = 10.0;
	Screen->Open(Now, "apartment");
	Now += 1.0;
	ss::ui::DrawList L = Frame(*Screen, 1920.0f, Now);
	Expect(Says(L, "YOUR CLOSET") && Says(L, "WEARING") && Says(L, "STYLE"), "closet: the slots, what's worn, the outfit's Style");
	Write("shop_wardrobe_portrait", L);
	// The jacket off (Take off), then back on (Wear): each Redress, and the closet changes.
	const std::string Jacket = S.Life.Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Outer)];
	Expect(!Jacket.empty(), "closet: the creator's bomber is worn");
	Screen->TakeRequests();
	Screen->Key("Alt", Now);
	std::vector<ss::ui::HostRequest> R = Screen->TakeRequests();
	Expect(S.Life.Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Outer)].empty() && Count(R, ss::ui::HostRequest::Redress) == 1, "closet: TAKE OFF the jacket (Redress)");
	Screen->Key("Right", Now);
	Screen->Key("Enter", Now);
	R = Screen->TakeRequests();
	Expect(!S.Life.Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Outer)].empty() && Count(R, ss::ui::HostRequest::Redress) == 1, "closet: WEAR puts one back on");
	Expect(Count(R, ss::ui::HostRequest::PreviewLook) >= 1, "closet: the choice in focus is previewed");
	Screen->Key("Tab", Now);
	R = Screen->TakeRequests();
	Expect(!Screen->IsOpen() && Screen->TakeLeave() && Count(R, ss::ui::HostRequest::EndPreview) == 1, "closet: DONE closes it and ends the preview");
}

// ------------------------------------------------------------------ the talk card

void Talk()
{
	ss::ui::TalkCard Card;
	double Now = 10.0;
	Card.Open(Now, "sal");
	ss::town::Talk T;
	T.Name = "Sal";
	T.Label = "Knows you";
	T.Line = "Dee's got a seat open. Tuesday, Thursday, Saturday. Bring cash, not excuses.";
	T.Choices = {{"news", "What's new?"}, {"games", "Any games around?"}, {"bye", "See you."}};
	Card.Show(Now, T);
	Card.Key("Enter", Now + 0.1);
	Expect(Card.TakeRequests().empty(), "talk: the grace window");
	Expect(Card.Typing(Now + 0.5), "talk: the line types on at 45 a second");
	ss::ui::DrawList L;
	{
		ss::ui::Canvas C(L, Measurer, 1920.0f, 1080.0f, 1.0f);
		Card.Draw(C, Now + 0.5);
	}
	Expect(Drew(L, "Sal") && Says(L, "KNOWS YOU") && !Says(L, "Bring cash"), "talk: the name, the label, the line half typed");
	Card.Key("Enter", Now + 0.6);
	Expect(!Card.Typing(Now + 0.6) && Card.TakeRequests().empty(), "talk: Enter while typing shows the whole line");
	Card.Key("Down", Now + 0.7);
	Card.Key("Enter", Now + 0.8);
	std::vector<ss::ui::HostRequest> R = Card.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ss::ui::HostRequest::Talk && R[0].Name == "games" && Card.Waiting(), "talk: a choice goes to the host");
	for (const float W : Widths)
	{
		ss::ui::DrawList Lw;
		ss::ui::Canvas C(Lw, Measurer, W, 1080.0f, 1.0f);
		Card.Draw(C, Now + 3.0);
		Fits(Lw, W, "talk card at " + std::to_string(static_cast<int>(W)));
		if (W == Widths[0])
		{
			Write("talk_card", Lw);
		}
	}
	T.Line = "The Garage, across the river, Thursdays at ten. Five-dollar big blind. Rick plays.";
	T.Choices = {{"news", "What's new?"}, {"bye", "See you."}};
	Card.Show(Now + 1.0, T);
	Card.Key("Down", Now + 5.0);
	Card.Key("Enter", Now + 5.0);
	R = Card.TakeRequests();
	Expect(R.size() == 2 && R[0].K == ss::ui::HostRequest::Talk && R[0].Name == "bye" && R[1].K == ss::ui::HostRequest::Leave && !Card.IsOpen() && Card.TakeLeave(), "talk: 'See you.' says so and leaves");
	Card.Open(Now + 6.0, "lonnie");
	Card.Show(Now + 6.0, T);
	Card.Key("Escape", Now + 6.1);
	R = Card.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ss::ui::HostRequest::Leave && Card.TakeLeave(), "talk: Escape walks away");
}

// ------------------------------------------------------------------ the street HUD

void Hud()
{
	Hooks H;
	ss::Session S(H, "shops-ui-hud");
	Career(S, 3, 1.5);
	for (const float W : Widths)
	{
		ss::ui::StreetHudInfo Info;
		Info.Place = "FIFTH & MARKET";
		Info.Clock = "1:30 AM";
		Info.World = S.WorldMinutes();
		Info.BankrollCents = S.BankrollCents;
		Info.Life = &S.Life;
		Info.Prompt = "Talk to Sal";
		Info.Weather = 3;
		Info.Rain = 0.7f;
		Info.Subtitles.push_back({"Marlene", "Coffee, hon? You look like you lost a flip.", 99.0});
		Info.Subtitles.push_back({"Sal", "Dee's got a seat open.", 100.0});
		Info.Subtitles.push_back({"Lonnie", "An old line nobody should see now.", 60.0});
		Info.NameTag.Text = "Sal";
		Info.NameTag.Sub = "Knows you";
		Info.NameTag.X = W * 0.5f;
		Info.NameTag.Y = 420.0f;
		Info.Closed = "Fifth St. Barbers \xC2\xB7 Closed \xC2\xB7 Opens Thu 10 AM";
		Info.ClosedAt = 100.0;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, W, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(C, Info, 101.0);
		const std::string What = "street HUD at " + std::to_string(static_cast<int>(W));
		Expect(Drew(L, "Rain") && Drew(L, "Dee's got a seat open.") && Drew(L, "Coffee, hon?") && !Drew(L, "An old line"), What + ": the weather and the two newest subtitles");
		Expect(Drew(L, "Sal") && Says(L, "KNOWS YOU"), What + ": the name tag");
		Expect(Says(L, "CLOSED") || Says(L, "Closed"), What + ": a locked door's line");
		Fits(L, W, What);
		if (W == Widths[0])
		{
			Write("street_hud_northside", L);
		}
		// The skip: the label, the bars, the clock in the corner; no prompt over a cinematic.
		Info.Skip.Active = true;
		Info.Skip.Label = "At Fifth St. Barbers";
		Info.Skip.Progress = 0.4f;
		Info.Skip.From = S.WorldMinutes();
		Info.Skip.To = S.WorldMinutes() + 50.0;
		ss::ui::DrawList Ls;
		ss::ui::Canvas Cs(Ls, Measurer, W, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(Cs, Info, 101.0);
		Expect(Says(Ls, "AT FIFTH ST. BARBERS") && Drew(Ls, "30 min to go") && !Says(Ls, "Talk to Sal"), What + ": the skip's overlay");
		Fits(Ls, W, What + " (skip)");
		if (W == Widths[0])
		{
			Write("street_hud_skip", Ls);
		}
	}
	// The Lucky Penny's clerk comes from the roster: Ray by day.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-clerk");
		Career(S2, 1, 14.0);
		ss::ui::StoreCounter Counter(S2);
		Counter.Open(10.0);
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, 1920.0f, 1080.0f, 1.0f);
		Counter.Draw(C, 11.0);
		Expect(Says(L, "RAY \xC2\xB7 OWNER"), "counter: Ray's behind the register at 2 PM");
		S2.LobbyMinutes = At(2, 2.0);
		ss::ui::DrawList L2;
		ss::ui::Canvas C2(L2, Measurer, 1920.0f, 1080.0f, 1.0f);
		Counter.Draw(C2, 12.0);
		Expect(Says(L2, "BENNY") && Says(L2, "NIGHT CLERK"), "counter: Benny at 2 AM");
	}
}

// ------------------------------------------------------------------ the faces

void Faces()
{
	Hooks H;
	ss::Session S(H, "shops-ui-faces");
	Career(S, 4, 12.0); // Friday noon: a ticket written at the pawn...
	const std::string Gear = GiveGear(S);
	Expect(S.PawnLoan("gear:" + Gear).empty() && !S.Life.Pawn.Tickets.empty(), "faces: a ticket to print");
	S.LobbyMinutes = At(4, 23.5); // ...then 11:30 PM: the barber open to midnight, the thrift shut, the Rialto's midnight show
	const int Ticket = S.Life.Pawn.Tickets.front().Number;
	const std::vector<std::pair<std::string, std::vector<std::string>>> Keys = {
		{"hours:barber", {"FIFTH ST.", "HOURS", "555-0118"}},
		{"hours:pawn", {"HOURS"}},
		{"hours:ginos", {"GINO'S"}},
		{"open:barber", {"OPEN"}},
		{"open:thrift", {"CLOSED", "OPENS"}},
		{"open:gym", {"MEMBERS ONLY"}},
		{"board:barber", {"FIFTH ST. BARBERS", "Crew Cut", "$22", "CUT & BEARD TOGETHER"}},
		{"board:diner", {"RUBY'S", "THE SHORT STACK", "$6.99"}},
		{"rack:outer", {"JACKETS"}},
		{"rack:hats", {"HATS & GLASSES"}},
		{"pawnticket:" + std::to_string(Ticket), {"PAWN TICKET", "REDEEM BY", "PLEDGOR'S COPY"}},
		{"pawncard:0", {"ALL SALES FINAL", "NO ID NO LOAN", "30-DAY LOANS", "TICKET REQUIRED TO REDEEM"}},
		{"pawncard:1", {"PLEASE WAIT", "FOR THE BUZZER"}},
		{"pawncard:2", {"WE BUY GOLD"}},
		{"whiteboard", {"WORKOUT OF THE DAY", "ROUNDS", "DAY PASS $10"}},
		{"backat", {"BACK AT", "6 AM"}},
		{"sandwich:diner", {"RUBY'S", "$6.99", "OPEN 24 HOURS"}},
		{"sandwich:thrift", {"YELLOW TAGS"}},
		{"sandwich:pawn", {"WE BUY", "GOLD", "GUITARS"}},
		{"sandwich:ginos", {"GINO'S"}},
		{"sandwich:kims", {"KIM'S"}},
		{"timetable:market-east", {"CITY TRANSIT", "EMBERCREST via MARKET", ":05", ":35"}},
		{"timetable:market-west", {"DOWNTOWN"}},
		{"shelterad", {"RiverLine", "the night shift never closes"}},
		{"dest:14 EMBERCREST", {"14", "EMBERCREST"}},
		{"destside:14 DOWNTOWN", {"DOWNTOWN"}},
		{"plate:" + ss::ui::shopart::PlateText(7), {ss::ui::shopart::PlateText(7)}},
		{"clock:" + std::to_string(S.WorldMinutes()), {"HOLLIS SAVINGS", "XII"}},
		{"marquee", {"MIDNIGHT SHOW FRI"}},
		{"valance:barber", {"FIFTH ST. BARBERS"}},
		{"valance:pharmacy", {"NORTHSIDE PHARMACY"}},
	};
	for (const auto& K : Keys)
	{
		float W = 0.0f;
		float Hh = 0.0f;
		Expect(ss::ui::shopart::FaceSize(K.first, W, Hh) && W > 0.0f && Hh > 0.0f, "faces: " + K.first + " has a size");
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, W, Hh, 1.0f);
		const bool Drawn = ss::ui::shopart::DrawFace(C, K.first, S);
		Expect(Drawn && !L.Cmds.empty(), "faces: " + K.first + " draws");
		for (const std::string& Needle : K.second)
		{
			Expect(Says(L, Needle), "faces: " + K.first + " says \"" + Needle + "\"");
		}
		Fits(L, W, "faces: " + K.first);
		std::string Name = K.first.substr(0, K.first.find(':'));
		if (K.first.find(':') != std::string::npos && Name != "plate" && Name != "clock" && Name != "pawnticket")
		{
			Name += "_" + K.first.substr(K.first.find(':') + 1);
		}
		for (char& Ch : Name)
		{
			Ch = Ch == ' ' || Ch == ':' ? '_' : Ch;
		}
		Write("face_" + Name, L);
	}
	float Nw = 0.0f;
	float Nh = 0.0f;
	Expect(!ss::ui::shopart::FaceSize("nothing:here", Nw, Nh), "faces: an unknown key has no size");
	// The trophy case's stamp: a bracelet on a ticket reads IN PAWN; on the shelf, nothing.
	{
		Hooks H2;
		ss::Session S2(H2, "shops-ui-award");
		Career(S2, 1, 12.0);
		S2.Living().GrantAward(-1, false, false);
		const std::vector<ss::world::Award>& Won = S2.Living().HeroAwards();
		if (!Won.empty())
		{
			const ss::world::Award A = Won.back();
			Expect(ss::ui::shopart::AwardStamp(S2, A).empty(), "faces: a bracelet at home has no stamp");
			Expect(S2.PawnLoan("award:" + std::to_string(A.Day) + ":" + A.Event).empty() && ss::ui::shopart::AwardStamp(S2, A) == "IN PAWN", "faces: a pawned bracelet is stamped IN PAWN");
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, Measurer, 300.0f, 300.0f, 1.0f);
			ss::ui::shopart::DrawAwardStamp(C, ss::ui::shopart::AwardStamp(S2, A), 150.0f, 150.0f, 200.0f);
			Expect(Says(L, "IN PAWN"), "faces: the stamp draws");
			// Letter by letter along the stamp's tilt, and inside the piece it's stamped on (here 200 across, at 150).
			float X0 = 1.0e9f, X1 = -1.0e9f, FirstBase = 0.0f, LastBase = 0.0f;
			int Letters = 0;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
				{
					X0 = std::min(X0, Cmd.Text.X);
					X1 = std::max(X1, Cmd.Text.X + Measurer.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx));
					FirstBase = Letters == 0 ? Cmd.Text.BaselineY : FirstBase;
					LastBase = Cmd.Text.BaselineY;
					++Letters;
				}
			}
			Expect(Letters == 7 && LastBase < FirstBase - 5.0f, "faces: the stamp's letters turn with it");
			Expect(X0 > 50.0f && X1 < 250.0f, "faces: the stamp stays on its piece");
			// A long word at a case's size shrinks to fit the piece rather than run into the next one.
			ss::ui::DrawList Small;
			ss::ui::Canvas Cs(Small, Measurer, 300.0f, 300.0f, 1.0f);
			ss::ui::shopart::DrawAwardStamp(Cs, "IN PAWN", 150.0f, 150.0f, 54.0f);
			float S0 = 1.0e9f, S1 = -1.0e9f;
			for (const ss::ui::DrawCmd& Cmd : Small.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
				{
					S0 = std::min(S0, Cmd.Text.X);
					S1 = std::max(S1, Cmd.Text.X + Measurer.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx));
				}
			}
			Expect(Says(Small, "IN PAWN") && S0 > 150.0f - 27.0f && S1 < 150.0f + 27.0f, "faces: at the trophy case's 54, IN PAWN fits its piece");
		}
	}
	// The pawn's string tags at its sockets: a curio at its shelf price, and a storage-auction lot tagged LOT 114.
	{
		Hooks H3;
		ss::Session S3(H3, "shops-ui-tags");
		Career(S3, 1, 12.0);
		std::vector<std::string> Lost;
		for (const ss::gear::Item& I : ss::gear::Catalog())
		{
			if (!I.Monthly && ss::shop::Takes(I) && Lost.size() < 3)
			{
				Lost.push_back(I.Id);
			}
		}
		// Unit 114 went three days ago: its lots reached Augie's shelf yesterday.
		ss::shop::Consign(S3.Life.Pawn, Lost, S3.WorldMinutes() - 3.0 * ss::net::MinutesPerDay, 11);
		std::string AuctionLot;
		for (const ss::shop::ShelfLot& Sh : S3.Life.Pawn.Shelf)
		{
			AuctionLot = Sh.Auction && AuctionLot.empty() ? Sh.Lot : AuctionLot;
		}
		Expect(!AuctionLot.empty(), "faces: the storage auction put a lot on the shelf");
		for (const std::string& Lot : {std::string("curio:aviators"), AuctionLot})
		{
			const std::string Key = "tag:" + Lot;
			ss::Chips Price = -1;
			for (const ss::shop::Lot& L : S3.PawnShelf())
			{
				Price = L.Id == Lot ? L.ValueCents : Price;
			}
			float W = 0.0f;
			float Hh = 0.0f;
			Expect(ss::ui::shopart::FaceSize(Key, W, Hh) && W > 0.0f && Hh > 0.0f, "faces: " + Key + " has a size");
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, Measurer, W, Hh, 1.0f);
			Expect(Price > 0 && ss::ui::shopart::DrawFace(C, Key, S3), "faces: " + Key + " draws, for what's on the shelf");
			Expect(Says(L, "$" + std::to_string(Price / 100)), "faces: " + Key + " carries the shelf's price (" + ss::Money(Price) + ")");
			Expect(Says(L, "LOT 114") == (Lot == AuctionLot), "faces: " + Key + (Lot == AuctionLot ? " is tagged LOT 114" : " carries no lot number"));
			Fits(L, W, "faces: " + Key);
			Write(Lot == AuctionLot ? "face_tag_lot114" : "face_tag_curio", L);
		}
		ss::ui::DrawList Lx;
		ss::ui::Canvas Cx(Lx, Measurer, 240.0f, 400.0f, 1.0f);
		Expect(!ss::ui::shopart::DrawFace(Cx, "tag:gear:nothing-like-it", S3), "faces: a tag for something not on the shelf draws nothing");
	}
	// Plates: seeded, never two alike in a lot of 40.
	std::vector<std::string> Lot;
	for (uint64_t K = 0; K < 40; ++K)
	{
		Lot.push_back(ss::ui::shopart::PlateText(K * 977 + 3));
	}
	std::sort(Lot.begin(), Lot.end());
	Expect(std::adjacent_find(Lot.begin(), Lot.end()) == Lot.end() && ss::ui::shopart::PlateText(5) == ss::ui::shopart::PlateText(5), "faces: plates are seeded and don't repeat");
	// The pictures: every garment on the racks and every lot draws.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, 1600.0f, 1000.0f, 1.0f);
		C.FillRect({0.0f, 0.0f, 1600.0f, 1000.0f}, ss::ui::Paint(ss::ui::Hex(0x10141c)));
		int I = 0;
		std::vector<int> Cuts;
		for (const ss::wardrobe::Garment& G : ss::wardrobe::Catalog())
		{
			if (std::find(Cuts.begin(), Cuts.end(), static_cast<int>(G.Kind)) != Cuts.end() || I >= 40)
			{
				continue;
			}
			Cuts.push_back(static_cast<int>(G.Kind));
			const size_t Before = L.Vertices.size();
			ss::ui::shopart::Garment(C, G, {20.0f + static_cast<float>(I % 8) * 196.0f, 20.0f + static_cast<float>(I / 8) * 196.0f, 180.0f, 180.0f});
			Expect(L.Vertices.size() > Before + 20, "faces: a " + G.Name + " draws");
			++I;
		}
		Expect(Cuts.size() >= 20, "faces: every cut of garment has a picture");
		Write("pictures_garments", L);
	}
}

// ------------------------------------------------------------------ the creator and the portrait: Outfit 6 "None"

struct MenuHooks : ss::ui::FrontEndHooks
{
};

void OutfitNone()
{
	Expect(ss::hero::OptionCount(ss::hero::Slot::Outfit) == ss::hero::OutfitNone + 1 && ss::hero::OptionName(ss::hero::Slot::Outfit, ss::hero::OutfitNone) == "None",
		"none: Outfit's seventh option is None");
	ss::hero::Character Who = Someone();
	auto Portrait = [&](int Outfit, int Color) {
		Who.Appearance.Outfit = Outfit;
		Who.Appearance.OutfitColor = Color;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, 600.0f, 700.0f, 1.0f);
		ss::ui::DrawPortrait(C, Who, 300.0f, 260.0f, 1.0f, 0.0);
		return L;
	};
	const ss::ui::DrawList Bomber = Portrait(1, 6);
	const ss::ui::DrawList Tee = Portrait(ss::hero::OutfitNone, 6);
	const ss::ui::DrawList Track = Portrait(5, 6);
	Expect(!Tee.Vertices.empty() && Tee.Vertices.size() != Track.Vertices.size() && Tee.Vertices.size() != Bomber.Vertices.size(), "none: the portrait draws the tee alone (not the track jacket)");
	// The tee is the outfit color (teal): find it among the portrait's colors.
	const ss::ui::Color Teal = ss::ui::Hex(ss::hero::OutfitTone(6));
	bool Found = false;
	for (const ss::ui::Vertex& V : Tee.Vertices)
	{
		Found = Found || (std::fabs(V.Col.R - Teal.R) < 0.12f && std::fabs(V.Col.G - Teal.G) < 0.12f && std::fabs(V.Col.B - Teal.B) < 0.12f && V.Col.A > 0.9f);
	}
	Expect(Found, "none: the tee is in the outfit color");
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, 1600.0f, 1000.0f, 1.0f);
		C.FillRect({0.0f, 0.0f, 1600.0f, 1000.0f}, ss::ui::Paint(ss::ui::Hex(0x10141c)));
		for (int K = 0; K < 8; ++K)
		{
			Who.Appearance.Outfit = ss::hero::OutfitNone;
			Who.Appearance.OutfitColor = K;
			Who.Appearance.Body = K % 2;
			Who.Appearance.Build = K % 4;
			ss::ui::DrawPortrait(C, Who, 110.0f + static_cast<float>(K) * 196.0f, 300.0f, 0.62f, 0.0);
		}
		Write("portrait_outfit_none", L);
	}
	// The creator's clothes tab: "No Jacket", and the color row is the tee's.
	MenuHooks Mh;
	ss::ui::FrontEnd Fe(Mh);
	double Now = 0.0;
	auto Settle = [&](double Seconds) {
		for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
		{
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, Measurer, 1920.0f, ss::ui::FrontEnd::Height, 1.0f);
			Now += 1.0 / 30.0;
			Fe.Draw(C, Now);
		}
	};
	Fe.Open(ss::ui::FrontEnd::Page::Attract, Now);
	Settle(1.0);
	Fe.Char('x', Now);
	Settle(1.0);
	Fe.Key("Enter", Now);
	Settle(0.5);
	// Someone to make: a name and a screen name, then on through the background to the look.
	Fe.Draft.FirstName = "Jesse";
	Fe.Draft.LastName = "Cole";
	Fe.ScreenName = "jcole";
	Fe.Key("Enter", Now);
	Settle(0.5);
	Fe.Key("Enter", Now);
	Settle(0.5);
	using Step = ss::ui::FrontEnd::CreatorStage;
	Expect(Fe.CreatorStep() == Step::Look, "none: the creator reaches the look");
	Fe.Key("Q", Now);
	Settle(0.5);
	Expect(Fe.CreatorTab() == 3, "none: Q from the face tab wraps to the clothes");
	Fe.Draft.Appearance.Outfit = ss::hero::OutfitNone;
	ss::ui::DrawList L;
	{
		ss::ui::Canvas C(L, Measurer, 1920.0f, ss::ui::FrontEnd::Height, 1.0f);
		Now += 1.0;
		Fe.Draw(C, Now);
	}
	Expect(Drew(L, "No Jacket") && Says(L, "TEE COLOR"), "none: the creator says No Jacket, and the color is the tee's");
	Write("creator_outfit_none", L);
	// Walking the outfit row reaches None and wraps past it.
	Fe.Draft.Appearance.Outfit = ss::hero::OutfitNone - 1;
	Fe.Draft.Appearance.Set(ss::hero::Slot::Outfit, Fe.Draft.Appearance.Outfit + 1, true);
	Expect(Fe.Draft.Appearance.Outfit == ss::hero::OutfitNone, "none: the row steps onto None");
	Fe.Draft.Appearance.Set(ss::hero::Slot::Outfit, Fe.Draft.Appearance.Outfit + 1, true);
	Expect(Fe.Draft.Appearance.Outfit == 0, "none: and wraps to the hoodie");
}

// ------------------------------------------------------------------ the review's regressions

/** Where the first glyph of a letter-spaced word was drawn (a button's tracked label), or (-1, -1). */
ss::ui::Vec2 SpelledAt(const ss::ui::DrawList& L, const std::string& Word)
{
	std::vector<const ss::ui::DrawCmd*> Runs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Runs.push_back(&Cmd);
		}
	}
	for (size_t I = 0; I < Runs.size(); ++I)
	{
		std::string Got;
		for (size_t K = I; K < Runs.size() && Got.size() < Word.size(); ++K)
		{
			Got += Runs[K]->Text.Text;
		}
		if (Got == Word)
		{
			return {Runs[I]->Text.X + 4.0f, Runs[I]->Text.BaselineY - Runs[I]->Text.SizePx * 0.35f};
		}
	}
	return {-1.0f, -1.0f};
}

void Review()
{
	// The tip row starts at the house's default every visit: last time's stiff isn't this time's choice.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-tip-default");
		Career(S, 1, 11.0);
		auto Sc = ss::ui::MakeShopScreen("barber", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "barber-chair1");
		T += 1.0;
		Sc->Key("Enter", T); // the hero's own style, cleaned up
		Sc->Key("Tab", T);
		Sc->Key("Left", T);
		Sc->Key("Left", T);
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T + 0.1);
		Expect(Says(L, "NO TIP"), "tip: Left twice from 20% is no tip");
		Sc->Key("Escape", T + 0.2); // off the tip row
		Sc->Key("Escape", T + 0.3); // out of the chair
		Expect(!Sc->IsOpen() && Sc->TakeLeave(), "tip: walked out without paying");
		Sc->Open(T + 1.0, "barber-chair1");
		T += 2.0;
		Sc->Key("Enter", T);
		Sc->Key("Tab", T);
		L = Frame(*Sc, 1920.0f, T + 0.1);
		Expect(Says(L, "TIP 20%") && !Says(L, "NO TIP"), "tip: the next visit's row starts at Lonnie's 20% again");
	}
	// In the chair the header doesn't quote the bench's wait; a beard with no shave on record says so.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-barber-chair");
		Career(S, 5, 11.0); // Saturday morning: the busiest bench of the week
		S.Life.Groom.ShaveAt = -1.0e9;
		auto Sc = ss::ui::MakeShopScreen("barber", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "BEARD");
		T += 1.0;
		const ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		Expect(!Says(L, "WAIT ABOUT"), "barber: in the chair, no queue in the header");
		Expect(Says(L, "No shave on Lonnie's books yet") && !Says(L, "days ago"), "barber: nothing on record reads as nothing, not a million days");
	}
	// Lonnie's books: every career starts with a cut and a shave a week back, from before the city. The first visit reads
	// as the first on both tabs; after a cut in his chair it's "Last cut today", and the beard tab quotes the shave on record.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-barber-books");
		Career(S, 1, 11.0);
		S.Life.Groom.CutAt = S.WorldMinutes() - 7.0 * ss::net::MinutesPerDay; // as NewCareer seeds it
		S.Life.Groom.ShaveAt = S.Life.Groom.CutAt;
		Expect(S.Life.Groom.Cuts == 0 && S.Life.Visits.count("barber") == 0, "barber: a new career, never in Lonnie's chair");
		auto Sc = ss::ui::MakeShopScreen("barber", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "CUT");
		T += 1.0;
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "First time in Lonnie's chair") && !Says(L, "Last cut"), "barber: the seeded cut isn't Lonnie's: first time in his chair");
		Sc->Key("E", T);
		L = Frame(*Sc, 1920.0f, T + 0.5);
		Expect(Says(L, "No shave on Lonnie's books yet") && !Says(L, "Last shave"), "barber: nor the seeded shave");
		Write("shop_barber_first_time", Frame(*Sc, 1920.0f, T + 0.6));
		Sc->Key("Escape", T + 0.7);
		// A line-up in his chair (what SettleShop leaves: the cut counted, the visit on his books).
		S.Life.Groom.CutAt = S.WorldMinutes();
		S.Life.Groom.Cuts = 1;
		S.Life.Visits["barber"].Count = 1;
		Sc->Open(T + 2.0, "CUT");
		L = Frame(*Sc, 1920.0f, T + 3.0);
		Expect(Says(L, "Last cut today") && !Says(L, "First time"), "barber: then the book says when his last cut was");
		Sc->Key("E", T + 3.0);
		L = Frame(*Sc, 1920.0f, T + 3.5);
		Expect(Says(L, "Last shave 7 days ago"), "barber: and the beard tab, the shave on record");
		// A shave and no cut: in his chair, but no cut on his books yet.
		S.Life.Groom.Cuts = 0;
		S.Life.Groom.CutAt = S.WorldMinutes() - 7.0 * ss::net::MinutesPerDay;
		Sc->Key("Q", T + 4.0);
		L = Frame(*Sc, 1920.0f, T + 4.5);
		Expect(Says(L, "No cut on Lonnie's books yet"), "barber: been in the chair for a shave, not a cut");
	}
	// The gym: a membership and a day pass never share a receipt.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-gym-both");
		Career(S, 2, 17.0);
		auto Sc = ss::ui::MakeShopScreen("gym", S);
		Sc->SetCaps(AllCaps());
		int Pass = -1;
		int Member = -1;
		int I = 0;
		for (const ss::shop::Offer& O : ss::shop::Offers("gym"))
		{
			if (O.Type == ss::shop::OfferKind::Membership || O.Type == ss::shop::OfferKind::DayPass || O.Type == ss::shop::OfferKind::Shake)
			{
				Pass = O.Type == ss::shop::OfferKind::DayPass ? I : Pass;
				Member = O.Type == ss::shop::OfferKind::Membership ? I : Member;
				++I;
			}
		}
		Expect(Pass >= 0 && Member >= 0, "gym: the desk sells both");
		double T = 10.0;
		Sc->Open(T, "gym-desk");
		T += 1.0;
		auto Go = [&](int To) {
			for (int K = 0; K < 8; ++K)
			{
				Sc->Key("Left", T);
			}
			for (int K = 0; K < To; ++K)
			{
				Sc->Key("Right", T);
			}
		};
		Go(Pass);
		Sc->Key("Enter", T);
		Go(Member);
		Sc->Key("Enter", T); // the membership takes the pass's place
		Go(Pass);
		Sc->Key("Enter", T); // and the pass won't go on beside it
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T + 0.4);
		Expect(Says(L, "Membership covers today"), "gym: Rosa won't sell a day pass with the membership");
		ss::Session::ShopOrder Only;
		Only.Venue = "gym";
		Only.Offers = {"gym/membership"};
		const ss::Chips Due = S.ShopTotal(Only);
		const ss::Chips Was = S.BankrollCents;
		Sc->Key("Tab", T + 0.5);
		Expect(Due > 0 && Was - S.BankrollCents == Due && S.Owns("gym"), "gym: one receipt, the membership alone (" + ss::Money(Due) + ")");
	}
	// Ruby's: a bottomless cup isn't rung up twice; and an order on the check isn't dropped by sitting down.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-diner-refill");
		Career(S, 3, 2.5);
		S.Life.CoffeeUntil = S.WorldMinutes() + 90.0;
		auto Sc = ss::ui::MakeShopScreen("diner", S);
		Sc->SetCaps(AllCaps());
		int Coffee = -1;
		int I = 0;
		for (const ss::shop::Offer& O : ss::shop::Offers("diner"))
		{
			if (O.Type == ss::shop::OfferKind::Meal || O.Type == ss::shop::OfferKind::Drink)
			{
				Coffee = O.Id == "diner/coffee" ? I : Coffee;
				++I;
			}
		}
		double T = 10.0;
		Sc->Open(T, "diner-stool1");
		T += 1.0;
		for (int K = 0; K < Coffee; ++K)
		{
			Sc->Key("Right", T);
		}
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "REFILLS FREE"), "diner: a cup going reads REFILLS FREE");
		Sc->Key("Enter", T);
		L = Frame(*Sc, 1920.0f, T + 0.5);
		Expect(Says(L, "bottomless") && Says(L, "ORDER SOMETHING"), "diner: a cup that's already bottomless isn't ordered again");
		Fits(L, 1920.0f, "diner: refills");
		const ss::Chips Was = S.BankrollCents;
		Sc->Key("Tab", T + 0.6);
		Sc->Key("Tab", T + 0.7);
		Expect(S.BankrollCents == Was && !S.TimeSkip.Active, "diner: nothing on the check, nothing charged");
	}
	{
		Hooks H;
		ss::Session S(H, "shops-ui-diner-sit-order");
		Career(S, 3, 2.5);
		auto Sc = ss::ui::MakeShopScreen("diner", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "diner-booth1");
		T += 1.0;
		Sc->Key("Enter", T); // a plate on the check
		Sc->Key("E", T);
		Sc->Key("E", T); // to SIT A WHILE
		const ss::Chips Was = S.BankrollCents;
		Sc->Key("Tab", T + 0.1);
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T + 0.5);
		Expect(Sc->IsOpen() && !S.TimeSkip.Active && S.BankrollCents == Was && Says(L, "SETTLE THE CHECK FIRST"), "diner: sitting a while waits for the check");
		Expect(Says(L, "still on the check"), "diner: the sit page says why");
		Fits(L, 1920.0f, "diner: sit with an order");
		Sc->Key("Q", T + 0.6);
		Sc->Key("Q", T + 0.6); // back to the menu: take the plate off, then sit
		Sc->Key("PutBack", T + 0.7);
		Sc->Key("E", T + 0.8);
		Sc->Key("E", T + 0.8);
		Sc->Key("Tab", T + 0.9);
		Expect(!Sc->IsOpen() && S.TimeSkip.Active && S.BankrollCents == Was, "diner: with the check clear, the booth is yours");
	}
	// Big Lou's Friday night (§6.3): for a player who cashed this week the guest check says he got it, the food and its
	// tax come off as his, and what's left to pay is the tip, exactly what the Session charges.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-diner-lou");
		Career(S, 4, 23.5, 100000);
		double World = -1.0;
		for (int D = 4; D < 120 && World < 0.0; D += 7)
		{
			S.LobbyMinutes = At(D, 23.5);
			S.Life.RentStage = ss::life::Rent::Paid;
			S.Life.RentDeadline = S.WorldMinutes() + 200.0 * ss::net::MinutesPerDay;
			S.Living().AdvanceTo(S.WorldMinutes());
			ss::town::Placement P;
			World = ss::town::Where("big-lou", S.WorldMinutes(), S, P) && P.Venue == "diner" && P.What == ss::town::Doing::Sit ? S.WorldMinutes() : -1.0;
		}
		Expect(World > 0.0, "lou: a Friday night with Big Lou in his booth at Ruby's");
		ss::life::LiveEntry E;
		E.Id = "embercrest-nightly@" + std::to_string(ss::net::DayOf(World) - 2);
		E.State = ss::life::LiveEntry::Finished;
		E.PrizeCents = 31000;
		E.FinishedAt = World - 2.0 * ss::net::MinutesPerDay;
		S.Life.LiveEntries.push_back(E);
		const std::string Key = ss::town::CheckKey(S, World);
		Expect(World > 0.0 && ss::town::PaysYourCheck(S, World) == "big-lou" && !Key.empty(), "lou: he's picking up the check tonight");
		auto Sc = ss::ui::MakeShopScreen("diner", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "diner-booth3");
		T += 1.0;
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		Expect(Says(L, "Big Lou's picking up your check tonight"), "lou: the menu says so before anything's ordered");
		for (const float W : Widths)
		{
			std::string Read = Joined(Frame(*Sc, W, T));
			std::replace(Read.begin(), Read.end(), '\n', ' ');
			Expect(Read.find("The tip's still yours.") != std::string::npos, "lou: the menu's aside reads in full at " + std::to_string(static_cast<int>(W)));
		}
		Sc->Key("Enter", T); // the Short Stack
		L = Frame(*Sc, 1920.0f, T + 0.1);
		const ss::Chips Food = 699 + ss::store::Tax(699);
		Expect(Says(L, "BIG LOU GOT IT") && Says(L, "\xE2\x88\x92" + ss::Money(Food)) && Says(L, "THE CHECK \xC2\xB7 BIG LOU GOT IT"),
			"lou: the check takes the food and its tax off as his (" + ss::Money(Food) + ")");
		for (const float W : Widths)
		{
			Fits(Frame(*Sc, W, T + 0.1), W, "lou: his check before the tip at " + std::to_string(static_cast<int>(W)));
		}
		Write("shop_diner_lou_check", Frame(*Sc, 1440.0f, T + 0.1));
		Sc->Key("Tab", T + 0.2); // to the tip
		L = Frame(*Sc, 1920.0f, T + 0.3);
		ss::Session::ShopOrder O;
		O.Venue = "diner";
		O.Offers = {"diner/short-stack"};
		O.TipPercent = 18;
		const ss::Chips Tip = (699 * 18 + 50) / 100;
		const ss::Chips Due = S.ShopTotal(O);
		Expect(Says(L, "TIP 18%") && Due == Tip && Says(L, "PAY THE TIP " + ss::Money(Tip)) && Says(L, "BIG LOU GOT IT"), "lou: the total is the tip alone (" + ss::Money(Tip) + ")");
		for (const float W : Widths)
		{
			Fits(Frame(*Sc, W, T + 0.3), W, "lou: the check at " + std::to_string(static_cast<int>(W)));
		}
		Write("shop_diner_lou", L);
		const ss::Chips Was = S.BankrollCents;
		const size_t Lines = S.Life.Ledger.size();
		Sc->Key("Tab", T + 0.4);
		Expect(Was - S.BankrollCents == Tip && S.Life.Ledger.size() == Lines + 2 && S.Life.Ledger[0].Label == "Ruby's (Big Lou got it)" && S.Life.Ledger[0].Amount == 0 &&
				   S.Life.Ledger[1].Amount == -Tip,
			"lou: paid: the tip through the card, his line for nothing in the Bank");
		Expect(S.Life.Said.count(Key) == 1 && ss::town::PaysYourCheck(S, World).empty() && !Sc->IsOpen() && S.TimeSkip.Active, "lou: one check a night, and the plate's on its way");
	}
	// Augie on Unit 114 names what really came in from it (the pawn book), at the electronics case.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-pawn-lot114");
		Career(S, 1, 12.0);
		const double World = S.WorldMinutes();
		for (const char* Id : {"tower-mid", "webcam-1080"})
		{
			ss::shop::ShelfLot Sh;
			Sh.Lot = std::string("gear:") + Id;
			Sh.PriceCents = ss::shop::GearValue(*ss::gear::Find(Id));
			Sh.From = World - ss::net::MinutesPerDay;
			Sh.Until = Sh.From + ss::shop::PawnShelfDays * ss::net::MinutesPerDay;
			Sh.Auction = true;
			S.Life.Pawn.Shelf.push_back(Sh);
		}
		auto Sc = ss::ui::MakeShopScreen("pawn", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "pawn-electronics");
		T += 1.0;
		const ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		std::string Read = Joined(L);
		std::replace(Read.begin(), Read.end(), '\n', ' ');
		Expect(ss::shop::AuctionWords(S.Life.Pawn, World) == "a desktop and a good webcam" && Read.find("Brought me a desktop and a good webcam.") != std::string::npos &&
				   Read.find("27-inch") == std::string::npos,
			"pawn: Augie names Unit 114's real pieces, not a stock pair");
		Write("shop_pawn_lot114", L);
	}
	// The pawn: buying back your own is a homecoming, and RENEW works with the mouse.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-pawn-home");
		Career(S, 1, 12.0);
		const std::string Gear = GiveGear(S);
		Expect(S.PawnSell("gear:" + Gear).empty() && !S.Owns(Gear), "pawn: sold to Augie");
		auto Sc = ss::ui::MakeShopScreen("pawn", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "YOURS");
		T += 1.0;
		Frame(*Sc, 1920.0f, T);
		Sc->Key("Enter", T);
		Sc->Key("Tab", T + 0.1);
		const ss::ui::DrawList L = Frame(*Sc, 1920.0f, T + 0.6);
		Expect(S.Owns(Gear) && Says(L, "It came home"), "pawn: buying back your own: \"Look at that. It came home.\"");
	}
	{
		Hooks H;
		ss::Session S(H, "shops-ui-pawn-renew");
		Career(S, 1, 12.0);
		const std::string Gear = GiveGear(S);
		Expect(S.PawnLoan("gear:" + Gear).empty() && S.Life.Pawn.Tickets.size() == 1, "pawn: a ticket to renew");
		auto Sc = ss::ui::MakeShopScreen("pawn", S);
		Sc->SetCaps(AllCaps());
		double T = 10.0;
		Sc->Open(T, "pawn-cage");
		T += 1.0;
		// Written today: nothing to renew yet. RENEW is greyed with the date it's good till, the hint drops it, and F does nothing.
		{
			ss::ui::DrawList Fresh = Frame(*Sc, 1920.0f, T);
			const ss::Chips Was = S.BankrollCents;
			Expect(Says(Fresh, "GOOD TILL") && !Says(Fresh, "RENEW 30 DAYS"), "pawn: a ticket written today shows RENEW greyed, good till its date");
			std::string Terms = Joined(Fresh);
			std::replace(Terms.begin(), Terms.end(), '\n', ' ');
			Expect(Terms.find("the loan plus twenty percent redeems one") != std::string::npos && Terms.find("renews it thirty more") != std::string::npos &&
					   Terms.find("twenty-five percent either way") != std::string::npos,
				"pawn: the TICKETS page states the terms, renewals and the grace's 25% included");
			Write("shop_pawn_tickets_fresh", Fresh);
			Sc->Key("F", T + 0.1);
			Expect(S.BankrollCents == Was && S.Life.Pawn.Tickets.front().Renewals == 0, "pawn: F on a fresh ticket pays nothing");
			for (const float W : Widths)
			{
				const ss::ui::DrawList Shown = Frame(*Sc, W, T + 0.2);
				Fits(Shown, W, "pawn: TICKETS with the terms at " + std::to_string(static_cast<int>(W)));
				// All of the terms, never cut short with an ellipsis: the last words are on the page.
				std::string Read = Joined(Shown);
				std::replace(Read.begin(), Read.end(), '\n', ' ');
				Expect(Read.find("the twenty alone renews it thirty more") != std::string::npos && Read.find("Then it's his.") != std::string::npos,
					"pawn: the terms read in full at " + std::to_string(static_cast<int>(W)));
			}
		}
		S.LobbyMinutes += 20.0 * ss::net::MinutesPerDay;
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		const ss::shop::Ticket Tk = S.Life.Pawn.Tickets.front();
		const ss::Chips Fee = ss::shop::RenewCents(Tk, S.WorldMinutes());
		Expect(Fee == ss::shop::RenewCents(Tk) && Says(L, "RENEW 30 DAYS \xC2\xB7 " + ss::Money(Fee)), "pawn: RENEW with its fee (the 20% in term) under REDEEM");
		for (const float W : Widths)
		{
			Fits(Frame(*Sc, W, T), W, "pawn: TICKETS with RENEW at " + std::to_string(static_cast<int>(W)));
		}
		Write("shop_pawn_tickets_renew", L);
		const ss::ui::Vec2 At_ = SpelledAt(L, "RENEW");
		Expect(At_.X > 0.0f, "pawn: the RENEW button is on screen");
		const ss::Chips Was = S.BankrollCents;
		Click(*Sc, 1920.0f, At_.X, At_.Y, T);
		L = Frame(*Sc, 1920.0f, T);
		Expect(Was - S.BankrollCents == Fee && S.Life.Pawn.Tickets.size() == 1 && S.Life.Pawn.Tickets.front().DueAt > Tk.DueAt + 19.0 * ss::net::MinutesPerDay,
			"pawn: a click on RENEW pays the 20% and starts the thirty days again");
	}
	// The thrift: what's picked at a rack is still in hand at the register; a day later it's back on the rack.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-thrift-carry");
		Career(S, 2, 14.0);
		auto Sc = ss::ui::MakeShopScreen("thrift", S);
		Sc->SetCaps(AllCaps());
		// The first jacket on the rack the hero doesn't own.
		const std::vector<std::string> Rack = ss::ui::shopart::ThriftRack(S, ~0ull);
		std::string Jacket;
		int Index = 0;
		int K = 0;
		for (const std::string& Id : Rack)
		{
			const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id);
			if (G && std::string(ss::ui::shopart::RackOf(*G)) == "outer")
			{
				if (Jacket.empty() && S.Life.Closet.Owned.count(Id) == 0)
				{
					Jacket = Id;
					Index = K;
				}
				++K;
			}
		}
		double T = 10.0;
		Sc->Open(T, "thrift-rack-outer");
		T += 1.0;
		for (int I = 0; I < Index; ++I)
		{
			Sc->Key("Right", T);
		}
		Sc->Key("Enter", T);
		Sc->Key("Escape", T + 0.1);
		Expect(!Sc->IsOpen() && Sc->TakeLeave(), "thrift: walked off the rack with a jacket in hand");
		S.LobbyMinutes += 2.0; // across the floor to Bea
		Sc->Open(T + 5.0, "thrift-register");
		T += 6.0;
		ss::ui::DrawList L = Frame(*Sc, 1920.0f, T);
		ss::Session::ShopOrder O;
		O.Venue = "thrift";
		O.Garments = {Jacket};
		const ss::Chips Due = S.ShopTotal(O);
		Expect(!Jacket.empty() && Due > 0 && Says(L, "PAY " + ss::Money(Due)) && Says(L, "ring you up"), "thrift: at the register, the jacket from the rack is on the receipt");
		Fits(L, 1920.0f, "thrift: the register");
		Write("shop_thrift_register", L);
		const ss::Chips Was = S.BankrollCents;
		Sc->Key("Tab", T + 0.1);
		Expect(Was - S.BankrollCents == Due && S.Life.Closet.Owned.count(Jacket) == 1, "thrift: paid at the register");
		Sc->Key("Escape", T + 0.2);
		S.LobbyMinutes += 24.0 * 60.0;
		Sc->Open(T + 10.0, "thrift-register");
		L = Frame(*Sc, 1920.0f, T + 11.0);
		Expect(!Says(L, "PAY $") && Says(L, "Racks are behind you"), "thrift: a day later the hands are empty");
	}
	// The skip overlay counts hours as hours.
	{
		Hooks H;
		ss::Session S(H, "shops-ui-hud-long");
		Career(S, 3, 1.5);
		ss::ui::StreetHudInfo Info;
		Info.Clock = "1:30 AM";
		Info.World = S.WorldMinutes();
		Info.Life = &S.Life;
		Info.Skip.Active = true;
		Info.Skip.Label = "Sitting a while at Ruby's";
		Info.Skip.From = S.WorldMinutes();
		Info.Skip.To = S.WorldMinutes() + 300.0;
		Info.Skip.Progress = 0.55f;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(C, Info, 10.0);
		Expect(Drew(L, "2 hr 15 min to go"), "hud: a long skip counts down in hours and minutes");
	}
}
// ------------------------------------------------------------------ captions (core-tableui, next_segment_spec §4.9)

/** DrawCaptions in its three modes at the three widths at 100% and 150%, and the night log with 40 lines. */
void Captions()
{
	using ss::ui::Caption;
	std::vector<Caption> Lines;
	auto Add = [&](int Style, const char* Who, const char* Line, double At, bool Room, int Side) {
		Caption C;
		C.Style = Style;
		C.Speaker = Who;
		C.Line = Line;
		C.At = At;
		C.Room = Room;
		C.Side = Side;
		Lines.push_back(C);
	};
	Add(1, "", "Ladies and gentlemen, we are on the bubble of the Embercrest Nightly. We're going hand for hand.", 97.0, false, 0);
	Add(0, "Rick", "Hand for hand. Here we go.", 98.0, false, 0);
	Add(0, "Corinne Vasquez", "All in and a call over here!", 98.5, true, 1);
	Add(2, "", "[The rail groans.]", 99.0, true, 2);
	Add(3, "Mrs. Park", "You going to make me wait all night, or are we playing cards?", 99.5, false, -1);
	for (const float W : Widths)
	{
		for (int Mode = 0; Mode < 3; ++Mode)
		{
			for (const int Size : {100, 150})
			{
				ss::ui::CaptionStyle Look;
				Look.Mode = Mode;
				Look.Size = Size;
				ss::ui::DrawList L;
				ss::ui::Canvas C(L, Measurer, W, 1080.0f, 1.0f);
				ss::ui::DrawCaptions(C, Lines, Look, 904.0f, std::min(980.0f, W - 840.0f), 100.0);
				const std::string What = "captions mode " + std::to_string(Mode) + " at " + std::to_string(Size) + "% at " + std::to_string(static_cast<int>(W));
				if (Mode == 0)
				{
					Expect(L.Cmds.empty(), What + ": off draws nothing");
				}
				else
				{
					Expect(Says(L, "FLOOR") && Says(L, "on the bubble"), What + ": the floor is never dropped");
					Expect(Mode == 2 || !Says(L, "[The rail groans.]"), What + ": sounds only in Everything");
				}
				Fits(L, W, What);
				if (W == Widths[0] && Mode == 1 && Size == 100)
				{
					Write("captions_dialogue", L);
				}
				if (W == Widths[0] && Mode == 2 && Size == 150)
				{
					Write("captions_everything_150", L);
				}
			}
		}
	}
	// The night log: forty lines, the clock in mono, the newest at the bottom.
	std::vector<Caption> Night;
	for (int K = 0; K < 40; ++K)
	{
		Caption C;
		C.Style = K % 6 == 0 ? 1 : K % 9 == 4 ? 2 : 0;
		C.Speaker = K % 2 == 0 ? "Mrs. Park" : "Big Lou";
		C.Line = C.Style == 1 ? "Players, the blinds are now 500 and 1,000, with a big blind ante of 1,000. Good luck, everyone." : C.Style == 2 ? "[Applause.]" : "Seat four, it's on you.";
		C.Clock = ss::ClockString(19.0 * 60.0 + static_cast<double>(K) * 3.0);
		Night.push_back(C);
	}
	Night.back().Line = "That's the last one tonight.";
	for (const float W : Widths)
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measurer, W, 1080.0f, 1.0f);
		const ss::ui::Rect R{80.0f, 140.0f, std::min(1300.0f, W - 160.0f), 760.0f};
		ss::ui::DrawNightLog(C, Night, "The night \xC2\xB7 Embercrest Nightly", R, 100.0);
		const std::string What = "night log at " + std::to_string(static_cast<int>(W));
		Expect(Says(L, "THE NIGHT \xC2\xB7 EMBERCREST NIGHTLY") && Drew(L, "That's the last one tonight.") && Drew(L, "8:57 PM"), What + ": the title and the newest line");
		Fits(L, W, What);
		if (W == Widths[0])
		{
			Write("night_log", L);
		}
	}
}
} // namespace shops_ui

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		shops_ui::OutDir = Argv[1];
	}
	shops_ui::EveryPage();
	shops_ui::Barber();
	shops_ui::Thrift();
	shops_ui::Gym();
	shops_ui::Diner();
	shops_ui::Pawn();
	shops_ui::Wardrobe();
	shops_ui::Talk();
	shops_ui::Hud();
	shops_ui::Faces();
	shops_ui::OutfitNone();
	shops_ui::Review();
	shops_ui::Captions();
	std::printf("shops_ui_test: %d checks, %d failed\n", shops_ui::Checks, shops_ui::Failures);
	return shops_ui::Failures == 0 ? 0 : 1;
}
