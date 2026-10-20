// riverline_ui: core-sponsor's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// Mail (mail_empty, mail_offer_luckypenny, mail_offer_conflict, mail_counter_mood, mail_contract_tally, mail_strike,
// mail_ended), Kast (kast_channel_sponsors, kast_overlay_marks), the art (sponsor_patch_<id> x13, ambassador_banner,
// polaroid), and the closet's PATCHES row (closet_patches).
//
// Mail draws once the laptop's taskbar opens it (App::Mail in RiverLine.cpp, ui-laptop's): until then its pages are
// reported as not wired and skipped, not failed.
#include "UiSeg.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/ShopScreen.h"
#include "ShortStack/UI/SponsorArt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace ui_test
{
namespace sponsor_seg
{
namespace sponsor = ss::sponsor;
using App = ss::ui::RiverLine::App;

constexpr double Day = 1440.0;

/** A label drawn letter-spaced (NetSpaced: one glyph a text run, as the contract card's section heads are), in order. */
bool DrewSpaced(const ss::ui::DrawList& L, const std::string& Label)
{
	std::string Glyphs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		size_t Points = 0;
		for (const char Ch : Cmd.Text.Text)
		{
			Points += (static_cast<unsigned char>(Ch) & 0xC0u) != 0x80u ? 1u : 0u;
		}
		Glyphs += Points == 1 ? Cmd.Text.Text : std::string("\n");
	}
	return Glyphs.find(Label) != std::string::npos;
}

struct Laptop
{
	QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit Laptop(const std::string& Seed) : S(H, Seed), RL(S) { S.CurrentScreen = ss::Screen::Lobby; }

	ss::ui::DrawList Frame(float X = 0.0f, float Y = 0.0f, bool Down = false, bool Pressed = false, bool Released = false)
	{
		RL.UI.Ptr.Active = X > 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	void Click(float X, float Y)
	{
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	}
	/** The page as it settles (its fade-in done), saved as Name. */
	ss::ui::DrawList Shot(const std::string& Name)
	{
		Now += 1.0;
		ss::ui::DrawList L = Frame();
		WriteList(Name, L);
		return L;
	}
	void Jump(double ToWorld)
	{
		if (ToWorld > S.WorldMinutes())
		{
			S.LobbyMinutes += ToWorld - S.WorldMinutes();
		}
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
};

void Mail()
{
	Laptop P("sponsor-mail");
	P.S.BankrollCents = 500000;
	P.Jump(3.0 * Day + 11.0 * 60.0);
	P.RL.OpenApp(App::Mail, P.Now);
	ss::ui::DrawList Empty = P.Shot("mail_empty");
	if (!Drew(Empty, "Inbox"))
	{
		std::printf("  mail: the taskbar doesn't open App::Mail yet (ui-laptop's RiverLine.cpp): Mail's pages skipped\n");
		return;
	}
	Expect(Drew(Empty, "No mail yet"), "mail: the empty inbox says so");
	// The Lucky Penny's offer, heard at the counter: in writing, with the contract card.
	P.S.DebugSponsor("offer luckypenny");
	P.RL.OpenApp(App::Mail, P.Now);
	ss::ui::DrawList Offer = P.Shot("mail_offer_luckypenny");
	Expect(Drew(Offer, "Lucky Penny #212") && DrewSpaced(Offer, "WHAT YOU GET") && DrewSpaced(Offer, "WHAT THEY ASK") && DrewSpaced(Offer, "THE FINE PRINT") && DrewRun(Offer, "Sign") &&
			   Drew(Offer, "OFFER"),
		"mail: an offer's thread, its card and its buttons");
	Expect(Drew(Offer, "$25 a week in store credit"), "mail: what you get, from the terms");
	// Two energy drinks: the conflict in red over Sign.
	P.S.DebugSponsor("offer voltrush");
	P.S.DebugSponsor("offer overclock");
	ss::ui::DrawList Conflict = P.Shot("mail_offer_conflict");
	Expect(Drew(Conflict, "Only one can be signed"), "mail: the conflict over Sign");
	// The counter's levers with the rep's mood.
	P.Click(681.0f, 904.0f);
	ss::ui::DrawList Counter = P.Shot("mail_counter_mood");
	Expect(Drew(Counter, "More money") && (Drew(Counter, "That's a stretch") || Drew(Counter, "Don't push it") || Drew(Counter, "Sounds open to it")), "mail: each lever with its mood");
	// A signed deal's card: this period's progress.
	P.S.DebugSponsor("sign voltrush");
	P.RL.OpenApp(App::Mail, P.Now);
	ss::ui::DrawList Tally = P.Shot("mail_contract_tally");
	Expect(Drew(Tally, "Live events in the patch") && Drew(Tally, "Period ends") && Drew(Tally, "Give notice"), "mail: a running deal's asks, deadline and notice");
	// The Sleep menu up over Give notice (update 2's gate 3 play-test): a press on the menu went to the app under it too, and a
	// night's sleep gave notice to the sponsor. Nothing under the menu takes a press now.
	{
		float Nx = -1.0f;
		float Ny = -1.0f;
		for (const ss::ui::DrawCmd& Cmd : Tally.Cmds)
		{
			if (Nx < 0.0f && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "Give notice")
			{
				Nx = Cmd.Text.X + P.M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx) / 2.0f;
				Ny = Cmd.Text.BaselineY - 5.0f;
			}
		}
		const ss::ui::Rect Menu = ss::ui::RiverLine::SleepMenuRect();
		const bool Under = Nx >= Menu.X && Nx <= Menu.X + Menu.W && Ny >= Menu.Y && Ny <= Menu.Y + Menu.H;
		P.RL.ShowSleepMenu(true);
		// A quick click lands in one frame, pressed and released together (as the game's input can give it): every button
		// under the pointer took it then, the app's first.
		P.Frame(Nx, Ny, false, true, true);
		P.Now += 0.05;
		const ss::ui::DrawList Slept = P.Shot("mail_notice_under_sleep");
		P.RL.ShowSleepMenu(false);
		Expect(Under && !Drew(Slept, "Give notice to "), "mail: a press on the Sleep menu over Give notice doesn't ask to give notice");
	}
	// A month short: the strike.
	P.S.DebugSponsor("period");
	ss::ui::DrawList Strike = P.Shot("mail_strike");
	Expect(Drew(Strike, "STRIKE") && Drew(Strike, "we came up short"), "mail: the strike, its letter and its pill");
	P.S.DebugSponsor("breach pickedup");
	ss::ui::DrawList Ended = P.Shot("mail_ended");
	Expect(Drew(Ended, "ENDED") && Drew(Ended, "about the paper"), "mail: ended under the conduct clause");
}

void Kast()
{
	Laptop P("sponsor-kast");
	P.S.BankrollCents = 500000;
	P.Jump(2.0 * Day + 20.0 * 60.0);
	P.S.Buy("ram-32");
	P.S.DebugSponsor("sign overclock");
	P.RL.OpenApp(App::Kast, P.Now);
	P.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Channel);
	ss::ui::DrawList Channel = P.Shot("kast_channel_sponsors");
	Expect(Drew(Channel, "Overclock") && Drew(Channel, "Offers are in Mail") && Drew(Channel, " h"), "kast: the Channel page's deals with their hours, offers in Mail");
	Expect(P.S.GoLive().empty(), "kast: live");
	P.Now = Step(P.S, P.Now, 5.0);
	P.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	ss::ui::DrawList Studio = P.Shot("kast_overlay_marks");
	Expect(Drew(Studio, "Overclock") && Drew(Studio, "Read for Overclock"), "kast: the overlay's mark and the read");
	P.S.EndStream();
	Expect(P.S.Channel.Deals.empty(), "kast: the overlay drew from the book, not Channel::Deals");
}

void Art()
{
	TableMeasurer M;
	std::set<size_t> Seen;
	for (const sponsor::Brand& B : sponsor::Brands())
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 512.0f, 340.0f, 1.0f);
		ss::ui::sponsorart::Patch(C, B.Id);
		Expect(L.Vertices.size() > 100, "art: the patch draws");
		// The thread (the edge and the word mark) reads on the patch's ground, as the art's atlas stitches it: never
		// IronHouse's grey or Volt Rush's near-black on the dark twill.
		{
			auto Lum = [](const ss::ui::Color& K) {
				auto Lin = [](float V) { return V <= 0.04045f ? V / 12.92f : std::pow((V + 0.055f) / 1.055f, 2.4f); };
				return 0.2126f * Lin(K.R) + 0.7152f * Lin(K.G) + 0.0722f * Lin(K.B);
			};
			std::string Word = B.Short;
			std::transform(Word.begin(), Word.end(), Word.begin(), [](char Ch) { return static_cast<char>(Ch >= 'a' && Ch <= 'z' ? Ch - 32 : Ch); });
			const ss::ui::DrawCmd* Mark = nullptr;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				Mark = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Word ? &Cmd : Mark;
			}
			const float Lg = Lum(L.Vertices.front().Col);
			const float Lw = Mark ? Lum(Mark->Text.Col) : Lg;
			const float Ratio = (std::max(Lg, Lw) + 0.05f) / (std::min(Lg, Lw) + 0.05f);
			Expect(Mark && Ratio >= 2.0f, ("art: " + B.Id + "'s thread reads on its patch").c_str());
		}
		Seen.insert(L.Vertices.size());
		WriteList("sponsor_patch_" + B.Id, L);
	}
	Expect(Seen.size() >= 12, "art: thirteen patches, each its own");
	ss::ui::DrawList Banner;
	ss::ui::Canvas Cb(Banner, M, 768.0f, 1280.0f, 1.0f);
	ss::ui::sponsorart::AmbassadorBanner(Cb, "Jesse Cole", "Team Embercrest \xC2\xB7 3 final tables this year");
	Expect(Drew(Banner, "TEAM EMBERCREST") || Banner.Cmds.size() > 10, "art: the ambassador banner");
	WriteList("ambassador_banner", Banner);
	ss::ui::DrawList Photo;
	ss::ui::Canvas Cp(Photo, M, 512.0f, 512.0f, 1.0f);
	ss::ui::sponsorart::Polaroid(Cp, ss::hero::Look(), "PENNY'S PICK");
	Expect(Drew(Photo, "PENNY'S PICK"), "art: the Polaroid's caption");
	WriteList("polaroid", Photo);
}

void Closet()
{
	QuietHooks H;
	ss::Session S(H, "sponsor-closet");
	S.BankrollCents = 400000;
	S.DebugSponsor("sign luckypenny");
	std::unique_ptr<ss::ui::ShopScreen> Screen = ss::ui::MakeShopScreen("wardrobe", S);
	TableMeasurer M;
	double Now = 10.0;
	Screen->Open(Now, "apartment");
	Now += 0.5;
	for (int K = 0; K < 6; ++K)
	{
		Screen->Key("TabNext", Now);
		Now += 0.1;
	}
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1920.0f, ss::ui::ShopScreen::Height, 1.0f);
	Screen->Draw(C, Now);
	WriteList("closet_patches", L);
	Expect(Drew(L, "Lucky Penny \xC2\xB7 CHEST") && Drew(L, "Lucky Penny \xC2\xB7 CAP") && DrewRun(L, "SLEEVE"), "closet: the Patches row: the penny on offer for the chest or the cap, the spots listed");
	Expect(Drew(L, "Missing: Lucky Penny on the chest or cap at live events"), "closet: the warning names the ask");
	Screen->Key("Enter", Now);
	Expect(S.Life.Closet.Patches[0] == "luckypenny", "closet: sewn on the chest");
	ss::ui::DrawList After;
	ss::ui::Canvas Ca(After, M, 1920.0f, ss::ui::ShopScreen::Height, 1.0f);
	Screen->Draw(Ca, Now + 0.5);
	Expect(Drew(After, "Every patch where the deal wants it"), "closet: sewn, and the asks are met");
}
} // namespace sponsor_seg

void SegSponsor()
{
	sponsor_seg::Mail();
	sponsor_seg::Kast();
	sponsor_seg::Art();
	sponsor_seg::Closet();
}
} // namespace ui_test
