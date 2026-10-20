// The rival sites on the laptop (update 2 §5.11, §1.8, §5.5-§5.7): which site the poker client wears and in whose
// colours, the poker button's list of the sites the player plays with their balances, a rival's PROMOTIONS and CASHIER
// pages, Career's RiverLine Rewards card, and the helpers the lobby, the tables, the Bank and Mail share (declared in
// RiverLineShared.h, rlsites_detail). ui-laptop.
//
// The money moves are core-sites' (Session::Deposit, Withdraw, AnswerInvite; site::Book): the pages here only read the
// book and call them, and every refusal shows in the session's own words.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/SitesDetail.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/SiteArt.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlsites_detail
{
/** RiverLine's 16 tokens in pal:: order (Ui.h; Sites.cpp's RiverLine SiteDef holds the same). */
const uint32_t SitesHomeHex[16] = {0x0a111c, 0x0e1726, 0x111c2e, 0x16233a, 0x223352, 0xe6edf7, 0x7b8aa3, 0x4d5b73, 0x27d3c3, 0x3b82f6, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e, 0x0f6a4e,
	0x083a2b};

/** The deposit steps (§5.11). */
const Chips SitesDepositSteps[4] = {1000, 5000, 10000, 50000};
/** The withdrawal steps; the last takes everything there is. */
const Chips SitesWithdrawSteps[4] = {2000, 5000, 10000, 20000};

// The copy the pages draw (every sentence is in SitesUiInventory, for the lint).
inline constexpr const char* SitesStaysThere = "Money on a site plays there and nowhere else. It never pays the rent.";
inline constexpr const char* SitesNothingComing = "Nothing on its way.";
inline constexpr const char* SitesDepositHow = "From your bank, by card. It lands at once.";
inline constexpr const char* SitesWithdrawHow = "Back to your bank. It takes a while, and they take their fee.";
inline constexpr const char* SitesOnlyBank = "Money only moves between a site and your bank, never site to site.";
inline constexpr const char* SitesNoLines = "Nothing here yet. Deposits, buy-ins and prizes show up as they happen.";
inline constexpr const char* SitesBonusRule = "It's released $5 at a time, for every $20 of rake you pay here, inside 60 days. Whatever's left after that expires.";
inline constexpr const char* SitesBonusStart = "Your first deposit starts it: 100% of it, up to $100.";
inline constexpr const char* SitesBonusDone = "All of it's yours now.";
inline constexpr const char* SitesTicketsRule = "A first deposit of $50 or more brings a $22 Sunday Grind ticket and two $5.50 daily tickets.";
inline constexpr const char* SitesTicketsNone = "None held right now.";
inline constexpr const char* SitesClubRule = "Rakeback into your balance here on the 1st, by the month's rake.";
inline constexpr const char* SitesNoLetters = "Nothing from them yet. Offers come by Mail.";
inline constexpr const char* SitesMenuNote = "Site money never pays the rent.";

// ------------------------------------------------------------------ what the book says

bool SitesRegistered(net::SiteId Site)
{
	if (Site == net::SiteId::RiverLine)
	{
		return true;
	}
	for (const net::SiteDef& D : net::Sites())
	{
		if (D.Id == Site)
		{
			return true;
		}
	}
	return false;
}

const site::Account* SitesAccount(const Session& S, net::SiteId Site)
{
	if (Site == net::SiteId::RiverLine)
	{
		return nullptr;
	}
	for (const site::Account& A : S.Life.Sites.Accounts)
	{
		if (A.Site == Site)
		{
			return &A;
		}
	}
	return nullptr;
}

std::vector<net::SiteId> SitesPlayed(const Session& S)
{
	std::vector<net::SiteId> Out = {net::SiteId::RiverLine};
	for (int I = 1; I < static_cast<int>(net::SiteId::Count); ++I)
	{
		const net::SiteId X = static_cast<net::SiteId>(I);
		if (SitesRegistered(X) && SitesAccount(S, X))
		{
			Out.push_back(X);
		}
	}
	return Out;
}

Chips SitesBalance(const Session& S, net::SiteId Site)
{
	if (Site == net::SiteId::RiverLine)
	{
		return S.BankrollCents;
	}
	const site::Account* A = SitesAccount(S, Site);
	return A ? A->Cents : 0;
}

Chips SitesInTransit(const Session& S, net::SiteId Site)
{
	Chips Sum = 0;
	for (const site::Transfer& T : S.Life.Sites.Transfers)
	{
		if (T.Out && T.State == 0 && (Site == net::SiteId::Count || T.Site == Site))
		{
			Sum += std::max<Chips>(0, T.Cents - T.FeeCents);
		}
	}
	return Sum;
}

Skin SitesSkin(net::SiteId Site)
{
	return Skin::FromHex(net::Def(SitesRegistered(Site) ? Site : net::SiteId::RiverLine).Palette);
}

bool SitesHome()
{
	static const Color Bg = Hex(SitesHomeHex[0]);
	static const Color Panel = Hex(SitesHomeHex[2]);
	static const Color Ink = Hex(SitesHomeHex[5]);
	static const Color Accent = Hex(SitesHomeHex[8]);
	const auto Same = [](const Color& A, const Color& B) { return A.R == B.R && A.G == B.G && A.B == B.B && A.A == B.A; };
	return Same(pal::Bg, Bg) && Same(pal::Panel, Panel) && Same(pal::Ink, Ink) && Same(pal::Accent, Accent);
}

Color SitesTone(uint32_t Rgb)
{
	if (SitesHome())
	{
		return Hex(Rgb);
	}
	// RiverLine's grounds, lines and inks as a ramp by lightness (Bg, Bg2, Panel, Panel2, Line, Dim, Muted, Ink): the literal
	// sits between two of them; it becomes the same point between the skin's two, plus whatever it strayed off the ramp.
	const int Order[8] = {0, 1, 2, 3, 4, 7, 6, 5};
	const Color* Now[16] = {&pal::Bg, &pal::Bg2, &pal::Panel, &pal::Panel2, &pal::Line, &pal::Ink, &pal::Muted, &pal::Dim, &pal::Accent, &pal::Accent2, &pal::Gold, &pal::Red, &pal::Orange, &pal::Green,
		&pal::Felt, &pal::FeltDark};
	const Color L = Hex(Rgb);
	const auto Light = [](const Color& C) { return (C.R + C.G + C.B) / 3.0f; };
	const float Lv = Light(L);
	int K = 0;
	while (K < 6 && Lv > Light(Hex(SitesHomeHex[Order[K + 1]])))
	{
		++K;
	}
	const Color A = Hex(SitesHomeHex[Order[K]]);
	const Color B = Hex(SitesHomeHex[Order[K + 1]]);
	const float Span = Light(B) - Light(A);
	const float T = std::min(1.0f, std::max(0.0f, Span > 0.0f ? (Lv - Light(A)) / Span : 0.0f));
	const Color Ramp = Mix(A, B, T);
	const Color Away = Mix(*Now[Order[K]], *Now[Order[K + 1]], T);
	const auto Clamp = [](float V) { return std::min(1.0f, std::max(0.0f, V)); };
	return {Clamp(Away.R + L.R - Ramp.R), Clamp(Away.G + L.G - Ramp.G), Clamp(Away.B + L.B - Ramp.B), 1.0f};
}

Color SitesOnAccent()
{
	return SitesHome() ? Hex(0x06201d) : Mix(pal::Accent, Hex(0x000000), 0.86f);
}

Color SitesAccentDeep()
{
	return SitesHome() ? Hex(0x1fb3a5) : Mix(pal::Accent, Hex(0x000000), 0.18f);
}

Color SitesOnGold()
{
	return SitesHome() ? Hex(0x231704) : Mix(pal::Gold, Hex(0x000000), 0.86f);
}

Color SitesGoldDeep()
{
	return SitesHome() ? Hex(0xd9a636) : Mix(pal::Gold, Hex(0x000000), 0.12f);
}

Color SitesFeltLight()
{
	return SitesHome() ? Hex(0x15875f) : Mix(pal::Felt, Hex(0xffffff), 0.12f);
}

/** The poker button's list: its rows and its size (PokerButton draws it; Draw keeps the screen under it still). */
const float SitesMenuRowH = 58.0f;
const float SitesMenuW = 360.0f;

Rect SitesMenuRect(const Session& S)
{
	const float H = 54.0f + SitesMenuRowH * static_cast<float>(SitesPlayed(S).size()) + 30.0f;
	const float ButtonY = RiverLine::Height - 38.0f + 4.0f;
	return {8.0f, ButtonY - 8.0f - H, SitesMenuW, H};
}

net::SiteId SitesOfTable(const Session& S)
{
	if (!S.T)
	{
		return net::SiteId::RiverLine;
	}
	const net::SiteId X = net::SiteOf(S.T->Spec.Id);
	return SitesRegistered(X) ? X : net::SiteId::RiverLine;
}

net::SiteId SitesShown(const RiverLine& RL, const Session& S)
{
	// The lobby's site, while the player still holds an account there (the laptop outlives a career: a new one has none).
	return RL.SiteShown != net::SiteId::RiverLine && SitesRegistered(RL.SiteShown) && SitesAccount(S, RL.SiteShown) ? RL.SiteShown : net::SiteId::RiverLine;
}

net::SiteId SitesClient(const RiverLine& RL, const Session& S)
{
	switch (S.CurrentScreen)
	{
	case Screen::Lobby: return SitesShown(RL, S);
	case Screen::Table: return SitesOfTable(S);
	case Screen::Results:
		if (S.HasResults && !S.History.empty() && S.History.front().Name == S.LastResults.EventName)
		{
			const net::SiteId X = net::SiteOf(S.History.front().EventId);
			return SitesRegistered(X) ? X : net::SiteId::RiverLine;
		}
		return net::SiteId::RiverLine;
	case Screen::Boot: break;
	}
	return net::SiteId::RiverLine;
}

std::string SitesShortName(net::SiteId Site)
{
	switch (Site)
	{
	case net::SiteId::Ignite: return "Ignite";
	case net::SiteId::RunItTwice: return "RunItTwice";
	case net::SiteId::Halcyon: return "Halcyon";
	default: return "RiverLine";
	}
}

std::string SitesLongName(net::SiteId Site)
{
	return Site == net::SiteId::Ignite ? std::string("Ignite Poker") : SitesShortName(Site);
}

std::string SitesUpperName(net::SiteId Site)
{
	return NetUpper(SitesShortName(Site));
}

net::HeroStats SitesHero(const Session& S, net::SiteId Site, const net::Network& Net, double World, net::Board For)
{
	// core-sites' numbers by site (bug 7): a rival's from its account and the results there; SiteBoard is this month's
	// points, which only Ignite's Month board reads.
	(void)Net;
	net::HeroStats H = site::HeroOn(Site, S.HeroName, S.History, S.Life.Sites, &S.Living(), World);
	if (For != net::Board::Month)
	{
		H.SiteBoard = 0.0;
	}
	return H;
}

std::vector<net::Board> SitesBoards(net::SiteId Site, bool SeriesLive)
{
	switch (Site)
	{
	case net::SiteId::Ignite: return {net::Board::Month, net::Board::Season};
	case net::SiteId::RunItTwice:
		if (SeriesLive)
		{
			return {net::Board::Season, net::Board::Wins, net::Board::Series};
		}
		return {net::Board::Season, net::Board::Wins};
	default: return {net::Board::Season};
	}
}

std::string SitesBoardName(net::SiteId Site, net::Board B)
{
	if (Site == net::SiteId::RiverLine)
	{
		return NetBoardName(B);
	}
	switch (B)
	{
	case net::Board::Month: return "This Month";
	case net::Board::Season: return "Season";
	case net::Board::Wins: return "Titles";
	case net::Board::Series: return "Series";
	case net::Board::Today: return "Today";
	case net::Board::Ladder: return "Ladder";
	case net::Board::HighRollers: return "High Rollers";
	default: return NetBoardName(B);
	}
}

/** Fills a template's {Key}s (the copy lint reads the templates with sample values). */
std::string SitesFill(std::string Text, const std::vector<std::pair<std::string, std::string>>& Vars)
{
	for (const std::pair<std::string, std::string>& V : Vars)
	{
		const std::string Key = "{" + V.first + "}";
		for (size_t At = Text.find(Key); At != std::string::npos; At = Text.find(Key, At + V.second.size()))
		{
			Text.replace(At, Key.size(), V.second);
		}
	}
	return Text;
}

std::string SitesBoardAbout(const Session& S, net::SiteId Site, net::Board B, double World)
{
	const std::string Name = SitesShortName(Site);
	const double Launched = S.Living().SiteLaunched(Site);
	const int Opened = Launched >= 0.0 ? net::DayOf(Launched) : net::NightOneDay;
	switch (B)
	{
	case net::Board::Month: return SitesFill(SitesAboutMonth, {{"Site", Name}});
	case net::Board::Season:
		// Its first season ran from the day its lobby opened; every rival's season starts again on January 1 after that
		// (the world's Sim::SitesNewYear).
		return world::YearOf(Opened) < world::YearOf(net::DayOf(World)) ? SitesFill(SitesAboutSeasonYear, {{"Site", Name}})
																		 : SitesFill(SitesAboutSeason, {{"Site", Name}, {"Date", net::DateLabel(Opened)}});
	case net::Board::Wins: return SitesFill(SitesAboutWins, {{"Site", Name}});
	case net::Board::Series:
	{
		const net::SeriesInfo* Sr = net::Site(Site).CurrentSeries(World);
		return SitesFill(SitesAboutSeries, {{"Series", Sr ? Sr->Name : std::string("the series")}});
	}
	default: break;
	}
	return SitesFill(SitesAboutAny, {{"Site", Name}});
}

std::string SitesClauseWarning(const Session& S, net::SiteId Site, bool Streaming, bool Video)
{
	if (!sponsor::WouldStrike(S.Life.Sponsors, static_cast<int>(Site), Streaming, Video))
	{
		return std::string();
	}
	// Whose deal it is: the running contract that plays only on another site.
	const double World = S.WorldMinutes();
	for (const sponsor::Contract& K : S.Life.Sponsors.Contracts)
	{
		if (K.T.Only.empty() || K.T.Only == net::KeyOf(Site) || !sponsor::Active(S.Life.Sponsors, K.Brand, World))
		{
			continue;
		}
		const sponsor::Brand* Br = sponsor::FindBrand(K.Brand);
		std::string Team = Br ? Br->Name : K.Brand;
		if (Br && Br->Site > 0 && Br->Site < static_cast<int>(net::SiteId::Count) && SitesRegistered(static_cast<net::SiteId>(Br->Site)))
		{
			const std::string& Named = net::Def(static_cast<net::SiteId>(Br->Site)).TeamName;
			Team = Named.empty() ? Team : Named;
		}
		return "Strike under your " + Team + " deal";
	}
	return "Strike under your sponsor's deal";
}

bool SitesButton(Ui& UI, Canvas& Cv, const std::string& Id, const Rect& R, const std::string& Label, const ButtonOpts& Opts)
{
	if (SitesHome())
	{
		return UI.Button(Id, R, Label, Opts);
	}
	// Ui::Button's layout, in the skin's own colours.
	const Ui::ClickState St = UI.Clickable(Id, R, Opts.Enabled);
	Color Base = SitesTone(0x1a2940);
	Color Hi = SitesTone(0x233756);
	Color Ink = pal::Ink;
	switch (Opts.Kind)
	{
	case ButtonKind::Primary:
		Base = Mix(pal::Accent, Hex(0x000000), 0.16f);
		Hi = pal::Accent;
		Ink = Mix(pal::Accent, Hex(0x000000), 0.86f);
		break;
	case ButtonKind::Gold:
		Base = Mix(pal::Gold, Hex(0x000000), 0.17f);
		Hi = pal::Gold;
		Ink = Mix(pal::Gold, Hex(0x000000), 0.86f);
		break;
	case ButtonKind::Danger:
		Base = Hex(0x7a2230);
		Hi = Hex(0x9b2b3c);
		Ink = Hex(0xffe9ec);
		break;
	case ButtonKind::Ghost:
		Base = Rgba(255, 255, 255, 0.02f);
		Hi = Rgba(255, 255, 255, 0.07f);
		break;
	case ButtonKind::Secondary: break;
	}
	const float Shift = St.Down ? 2.0f : 0.0f;
	const float Prev = Cv.GetAlpha();
	Cv.SetAlpha(Opts.Enabled ? Prev : Prev * 0.35f);
	const Rect Body{R.X, R.Y + Shift, R.W, R.H};
	Cv.FillRoundRect(Body, 10.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, St.Hover ? Hi : Base, Base));
	if (Opts.Kind == ButtonKind::Ghost)
	{
		Cv.StrokeRoundRect(Body, 10.0f, Rgba(255, 255, 255, 0.12f), 1.0f);
	}
	const float Size = Opts.Size;
	const float Cy = R.Y + R.H / 2.0f + Shift;
	if (!Opts.Sub.empty())
	{
		Cv.Text(Label, R.X + R.W / 2.0f, Cy - 4.0f, Ts(Size, 700, Ink, Align::Center));
		Cv.Text(Opts.Sub, R.X + R.W / 2.0f, Cy + Size * 0.85f, Ts(Size * 0.72f, 600, Ink, Align::Center, Baseline::Alphabetic, true));
	}
	else
	{
		Cv.Text(Label, R.X + R.W / 2.0f, Cy + 1.0f, Ts(Size, 700, Ink, Align::Center, Baseline::Middle));
	}
	if (!Opts.Hotkey.empty())
	{
		Cv.Text(Opts.Hotkey, R.X + R.W - 10.0f, R.Y + 16.0f, Ts(12.0f, 700, Ink, Align::Right, Baseline::Middle, true));
	}
	Cv.SetAlpha(Prev);
	return St.Clicked;
}

std::map<const RiverLine*, SitesUi>& SitesStates()
{
	static std::map<const RiverLine*, SitesUi> ByLaptop;
	return ByLaptop;
}

SitesUi& SitesOf(const RiverLine* RL)
{
	return SitesStates()[RL];
}

void ForgetSites(const RiverLine* RL)
{
	SitesStates().erase(RL);
}

bool SitesMenuOpen(const RiverLine* RL)
{
	const auto It = SitesStates().find(RL);
	return It != SitesStates().end() && It->second.MenuOpen;
}

void SitesMenuToggle(const RiverLine* RL, bool Open)
{
	SitesOf(RL).MenuOpen = Open;
}

std::string SitesRentLine(const Session& S, double World)
{
	const life::State& L = S.Life;
	if (L.RentStage == life::Rent::Evicted || L.RentStage == life::Rent::Paid)
	{
		return std::string();
	}
	std::string Line;
	if (site::RentWarning(L.Sites, S.BankrollCents, L.RentDueCents, L.RentDeadline, World, Line) && !Line.empty())
	{
		return Line;
	}
	return std::string();
}

bool SitesPlayedRival(const Session& S)
{
	for (const site::Account& A : S.Life.Sites.Accounts)
	{
		if (A.LifetimeEvents > 0)
		{
			return true;
		}
	}
	for (const HistoryEntry& E : S.History)
	{
		if (!E.EventId.empty() && net::SiteOf(E.EventId) != net::SiteId::RiverLine)
		{
			return true;
		}
	}
	return false;
}

/** A site's way in (§5.5's P0 card) and its way out (the transfer back to the bank): core-sites' methods. */
const net::CashMethod* SitesDepositMethod(net::SiteId Site)
{
	return site::MethodFor(Site, std::string(), false);
}

const net::CashMethod* SitesWithdrawMethod(net::SiteId Site)
{
	return site::MethodFor(Site, std::string(), true);
}

/** "Mon to Wed" for a window of hours from World (or one day's name when the window is a day). */
std::string SitesWindow(double From, double HoursMin, double HoursMax)
{
	const int A = net::DayOf(From + HoursMin * 60.0);
	const int B = net::DayOf(From + HoursMax * 60.0);
	if (A == B)
	{
		return std::string(net::WeekdayName(A)) + " " + net::TimeLabel(From + HoursMin * 60.0);
	}
	return std::string(net::WeekdayName(A)) + " to " + net::WeekdayName(B);
}

/** The fee a deposit of Cents pays by M (permille, rounded to the cent). */
Chips SitesDepositFee(const net::CashMethod& M, Chips Cents)
{
	return static_cast<Chips>(std::llround(static_cast<double>(Cents) * static_cast<double>(M.FeePermille) / 1000.0));
}

/** "8%" or "5.5%": a tier's share, from its per mille. */
std::string SitesPct(int Permille)
{
	return Fixed(static_cast<double>(Permille) / 10.0, Permille % 10 == 0 ? 0 : 1) + "%";
}

/** A site's letters in Mail, newest first. */
std::vector<const life::Letter*> SitesLetters(const Session& S, net::SiteId Site);
net::SiteId SitesOfLetter(const life::Letter& M);

net::SiteId SitesOfMail(const std::string& Source, const std::string& Thread, const std::string& From)
{
	life::Letter M;
	M.Source = Source;
	M.Thread = Thread;
	M.From = From;
	return SitesOfLetter(M);
}

net::SiteId SitesOfLetter(const life::Letter& M)
{
	if (M.Source != "site")
	{
		return net::SiteId::Count;
	}
	for (int I = 0; I < static_cast<int>(net::SiteId::Count); ++I)
	{
		const net::SiteId X = static_cast<net::SiteId>(I);
		const std::string Key = net::KeyOf(X);
		if (M.Thread == Key || (M.Thread.size() > Key.size() && M.Thread.compare(0, Key.size(), Key) == 0 &&
								   (M.Thread[Key.size()] == ':' || M.Thread[Key.size()] == '.' || M.Thread[Key.size()] == '-')))
		{
			return X;
		}
	}
	for (int I = 0; I < static_cast<int>(net::SiteId::Count); ++I)
	{
		const net::SiteId X = static_cast<net::SiteId>(I);
		if (SitesRegistered(X) && (M.From == net::Def(X).Rep || M.From == net::Def(X).Name))
		{
			return X;
		}
	}
	return net::SiteId::RiverLine;
}

std::vector<const life::Letter*> SitesLetters(const Session& S, net::SiteId Site)
{
	std::vector<const life::Letter*> Out;
	for (auto It = S.Life.Inbox.rbegin(); It != S.Life.Inbox.rend(); ++It)
	{
		if (SitesOfLetter(*It) == Site)
		{
			Out.push_back(&*It);
		}
	}
	return Out;
}

/** The invitation a site sent, if any (the latest). */
const site::Invitation* SitesInvite(const Session& S, net::SiteId Site)
{
	const site::Invitation* Out = nullptr;
	for (const site::Invitation& I : S.Life.Sites.Invites)
	{
		Out = I.Site == Site && (!Out || I.At >= Out->At) ? &I : Out;
	}
	return Out;
}
} // namespace rlsites_detail

using namespace rlsites_detail;

// ------------------------------------------------------------------ the poker button

void RiverLine::PokerButton(const Rect& R, double Now)
{
	// The taskbar's first button: the site in front's mark and name (RiverLine's own icon at home), and once a rival
	// account is open, a caret that lists every site played with its balance (§5.11, §1.9 #9).
	const net::SiteId Site = AppShown == App::RiverLine ? SitesClient(*this, S) : SitesShown(*this, S);
	const bool Caret = SitesPlayed(S).size() > 1;
	const float CaretW = Caret ? 20.0f : 0.0f;
	const Rect Main{R.X, R.Y, R.W - CaretW, R.H};
	const Ui::ClickState St = UI.Clickable(Site == net::SiteId::RiverLine ? std::string("appRiverLine") : "app" + SitesShortName(Site), Main);
	if (St.Clicked)
	{
		OpenApp(App::RiverLine, Now);
		SitesMenuToggle(this, false);
	}
	const bool On = AppShown == App::RiverLine;
	const bool Menu = SitesMenuOpen(this);
	if (On || St.Hover || Menu)
	{
		UI.RRect(R, 8.0f, Rgba(255, 255, 255, On || Menu ? 0.09f : 0.05f));
	}
	if (Site == net::SiteId::RiverLine)
	{
		AppIcon(App::RiverLine, R.X + 6.0f, R.Y + 3.0f, 24.0f);
	}
	else
	{
		siteart::Logo(*C, {R.X + 6.0f, R.Y + 3.0f, 24.0f, 24.0f}, Site);
	}
	UI.Text(SitesShortName(Site), R.X + 38.0f, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 700, On ? pal::Ink : Hex(0x9aa7bd), Align::Left, Baseline::Middle));
	if (On)
	{
		const uint32_t Line = Site == net::SiteId::RiverLine ? 0x27d3c3 : net::Def(Site).Palette[8];
		UI.RRect({R.X + 8.0f, R.Y + R.H + 1.0f, R.W - 16.0f - CaretW, 2.0f}, 1.0f, Hex(Line));
	}
	if (!Caret)
	{
		return;
	}
	// The caret: its own hit area at the button's right.
	const Rect Cr{R.X + R.W - CaretW - 2.0f, R.Y, CaretW + 2.0f, R.H};
	const Ui::ClickState Cs = UI.Clickable("pokersites", Cr);
	if (Cs.Clicked)
	{
		SitesMenuToggle(this, !Menu);
		SitesOf(this).MenuAt = Now;
		SleepOpen = false;
	}
	if (Cs.Hover)
	{
		UI.RRect({Cr.X + 2.0f, Cr.Y + 3.0f, Cr.W - 4.0f, Cr.H - 6.0f}, 6.0f, Rgba(255, 255, 255, 0.06f));
	}
	C->FillRect({Cr.X, Cr.Y + 7.0f, 1.0f, Cr.H - 14.0f}, Rgba(255, 255, 255, 0.1f));
	NetChevron(*C, Cr.X + Cr.W / 2.0f + 1.0f, R.Y + R.H / 2.0f, 9.0f, Menu ? 3 : 2, Menu || Cs.Hover ? pal::Ink : Hex(0x9aa7bd));
	if (!Menu)
	{
		return;
	}
	// The list, above the taskbar: every site played, its balance, what's on its way back.
	const std::vector<net::SiteId> Played = SitesPlayed(S);
	const float RowH = SitesMenuRowH;
	const Rect Pn = SitesMenuRect(S);
	if (UI.Ptr.Released && !UI.Hover(Pn) && !UI.Hover(R))
	{
		SitesMenuToggle(this, false);
		return;
	}
	const float In = NetEase((Now - SitesOf(this).MenuAt) / 0.2);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * In);
	C->GlowRoundRect({Pn.X, Pn.Y + 8.0f, Pn.W, Pn.H}, 14.0f, Rgba(0, 0, 0, 0.55f), 22.0f);
	UI.RRect(Pn, 14.0f, Paint::Linear({0.0f, Pn.Y}, {0.0f, Pn.Y + Pn.H}, Hex(0x131c2b), Hex(0x0b1220)), Hex(0x24324a));
	NetSpaced(*C, "POKER SITES", Pn.X + 18.0f, Pn.Y + 30.0f, 10.5f, 900, Hex(0x8a97ad), 1.6f);
	const Chips Coming = SitesInTransit(S);
	if (Coming > 0)
	{
		UI.Text(NetMoney(Coming) + " arriving", Pn.X + Pn.W - 18.0f, Pn.Y + 30.0f, Ts(12.0f, 700, Hex(0x9fb6d6), Align::Right));
	}
	float Y = Pn.Y + 44.0f;
	for (const net::SiteId X : Played)
	{
		const Rect Row{Pn.X + 8.0f, Y, Pn.W - 16.0f, RowH - 4.0f};
		const Ui::ClickState Rs = UI.Clickable("pokersite" + std::string(net::KeyOf(X)), Row);
		const bool Here = X == Site && On;
		if (Rs.Hover || Here)
		{
			UI.RRect(Row, 10.0f, Rgba(255, 255, 255, Here ? 0.08f : 0.05f));
		}
		siteart::Logo(*C, {Row.X + 10.0f, Row.Y + 9.0f, 36.0f, 36.0f}, X);
		UI.Text(SitesLongName(X), Row.X + 58.0f, Row.Y + 24.0f, Ts(15.0f, 800, Hex(0xe6edf7), Align::Left, Baseline::Alphabetic, false, 170.0f));
		const Chips InTransit = X == net::SiteId::RiverLine ? 0 : SitesInTransit(S, X);
		const std::string Sub = X == net::SiteId::RiverLine ? std::string("Your bank: it pays the rent") : InTransit > 0 ? NetMoney(InTransit) + " on its way back" : std::string("Wallet");
		UI.Text(Sub, Row.X + 58.0f, Row.Y + 43.0f, Ts(12.0f, 600, Hex(0x8a97ad), Align::Left, Baseline::Alphabetic, false, 190.0f));
		UI.Text(Money(SitesBalance(S, X)), Row.X + Row.W - 12.0f, Row.Y + 33.0f, Ts(16.0f, 800, Hex(0xf2c14e), Align::Right, Baseline::Alphabetic, true));
		if (Rs.Clicked)
		{
			SiteShown = X;
			SitesMenuToggle(this, false);
			OpenApp(App::RiverLine, Now);
			if (S.CurrentScreen != Screen::Boot)
			{
				S.ShowLobby();
				OpenPage(Page::Lobby, Now);
			}
		}
		Y += RowH;
	}
	UI.Text(SitesMenuNote, Pn.X + 18.0f, Pn.Y + Pn.H - 14.0f, Ts(12.0f, 600, Hex(0x8a97ad)));
	C->SetAlpha(A0);
}

// ------------------------------------------------------------------ Career's Rewards card

void RiverLine::RewardsCard(const Rect& R, double Now)
{
	// RiverLine Rewards (§5.7): the tier the month's rake has reached so far, the share it pays back on the 1st, how far to
	// the next, and the retention offer's bump while it runs ("We noticed.": one tier up for 60 days).
	const site::Rewards& Rw = S.Life.Sites.RiverLine;
	const std::vector<site::TierInfo>& Tiers = site::RewardsTiers();
	const Chips Rake = Rw.MonthKey == site::MonthKeyAt(World) ? Rw.Rake : 0;
	const int Earned = site::TierFor(Tiers, Rake);
	const int Top = static_cast<int>(Tiers.size()) - 1;
	// The bump counts when the month is paid (core-sites pays a month one tier up only if its 1st falls inside the boost), so
	// the card shows the tier the 1st will pay, never one the boost won't reach.
	const bool Boosted = Rw.BoostUntil > 0.0 && site::NextMonthStart(World) <= Rw.BoostUntil && Earned < Top;
	const int Tier = Boosted ? Earned + 1 : Earned;
	const site::TierInfo& T = Tiers[static_cast<size_t>(Tier)];
	const float In = NetEase((Now - PageAt) / 0.9);
	const Color Tc = Tier >= 4 ? Hex(0xb9e6ff) : Tier == 3 ? Hex(0xd7dde8) : Tier == 2 ? pal::Gold : Tier == 1 ? Hex(0xc9d3e0) : Hex(0xd08c4f);
	UI.RRect(R, 12.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(SitesTone(0x101a2b), Tc, 0.1f), SitesTone(0x101a2b)), pal::Line);
	C->FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, Tc);
	NetSpaced(*C, "RIVERLINE REWARDS", R.X + 18.0f, R.Y + 24.0f, 9.5f, 800, pal::Muted, 1.4f);
	const float Nw = UI.Text(T.Name, R.X + 18.0f, R.Y + 52.0f, Ts(20.0f, 900, Tc));
	UI.Text(SitesPct(T.Permille) + " back", R.X + 26.0f + Nw, R.Y + 52.0f, Ts(13.5f, 700, pal::Ink));
	if (Boosted)
	{
		const std::string Pill = "+1 TIER TO " + NetUpper(net::DateLabel(net::DayOf(Rw.BoostUntil)));
		NetPill(*C, Pill, R.X + R.W - 18.0f - C->Measure(Pill, 9.0f, 700) - 9.0f * 1.3f, R.Y + 12.0f, pal::Gold, false, 9.0f);
	}
	else
	{
		UI.Text("Paid on " + net::DateLabel(net::DayOf(site::NextMonthStart(World))), R.X + R.W - 18.0f, R.Y + 24.0f, Ts(12.0f, 700, pal::Muted, Align::Right));
	}
	// The month so far and the next step.
	const Rect Bar{R.X + 18.0f, R.Y + 64.0f, R.W - 36.0f, 6.0f};
	UI.RRect(Bar, 3.0f, Rgba(255, 255, 255, 0.07f));
	std::string Line = Money(Rake) + " of RiverLine rake this month";
	if (Earned < Top)
	{
		const site::TierInfo& Next = Tiers[static_cast<size_t>(Earned + 1)];
		const Chips From = Tiers[static_cast<size_t>(Earned)].From;
		const float Frac = Nf(Clamp01(static_cast<double>(Rake - From) / static_cast<double>(std::max<Chips>(1, Next.From - From))));
		UI.RRect({Bar.X, Bar.Y, std::max(6.0f, Bar.W * Frac * In), Bar.H}, 3.0f, Tc);
		Line += " \xC2\xB7 " + Money(Next.From - Rake) + " to " + Next.Name;
	}
	else
	{
		UI.RRect(Bar, 3.0f, Tc);
	}
	UI.Text(Line, R.X + 18.0f, R.Y + 88.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
}

// ------------------------------------------------------------------ a rival's Promotions page

void RiverLine::SitePromotions(double Now)
{
	const net::SiteId Site = SitesClient(*this, S);
	const site::Account* A = SitesAccount(S, Site);
	const std::string Name = SitesShortName(Site);
	const double Since = Now - PageAt;
	const float In = NetEase(Since / 0.6);
	Section("PROMOTIONS", 24.0f, 108.0f, pal::Accent);
	UI.Text("What " + Name + " gives back", 24.0f, 150.0f, Ts(30.0f, 900, pal::Ink));

	// The offers the site runs for the player (left), and its letters (right).
	const Rect Big{24.0f, 176.0f, 1052.0f, 330.0f};
	Panel(Big);
	const float Lx = Big.X + 30.0f;
	if (Site == net::SiteId::Ignite)
	{
		NetSpaced(*C, "WELCOME BONUS", Lx, Big.Y + 40.0f, 11.0f, 800, pal::Accent, 1.6f);
		UI.Text("100% of your first deposit, up to $100", Lx, Big.Y + 80.0f, Ts(26.0f, 900, pal::Ink));
		NetParagraph(*C, SitesBonusRule, Lx, Big.Y + 110.0f, 620.0f, 15.0f, 500, SitesTone(0xc3cedf), 21.0f, 3);
		const Chips Pending = A ? A->BonusPending : 0;
		const Chips Released = A ? A->BonusReleased : 0;
		const Chips Total = Pending + Released;
		// The first deposit started it (core-sites dates its end); once the 60 days are up core-sites zeroes what's pending,
		// so what was matched is gone and the page says the window closed rather than "all of it's yours".
		const bool Started = A && A->BonusExpires > 0.0;
		const bool Closed = Started && Pending == 0 && World >= A->BonusExpires;
		const Rect Bar{Lx, Big.Y + 196.0f, Big.W - 60.0f, 14.0f};
		UI.RRect(Bar, 7.0f, Rgba(255, 255, 255, 0.06f));
		if (Started)
		{
			if (!Closed && Total > 0)
			{
				const float Frac = Nf(Clamp01(static_cast<double>(Released) / static_cast<double>(Total)));
				UI.RRect({Bar.X, Bar.Y, std::max(14.0f, Bar.W * Frac * In), Bar.H}, 7.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, pal::Accent, pal::Accent2));
			}
			UI.Text(Money(Released) + " released", Lx, Big.Y + 244.0f, Ts(20.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true));
			const std::string State = Pending > 0 ? Money(Pending) + " still pending"
				: Closed ? SitesFill(SitesBonusClosed, {{"Date", net::DateLabel(net::DayOf(A->BonusExpires))}})
						 : std::string(SitesBonusDone);
			UI.Text(State, Lx + 260.0f, Big.Y + 244.0f, Ts(15.0f, 700, pal::Ink));
			if (Pending > 0 && A)
			{
				const double Left = A->BonusExpires - World;
				UI.Text(Left > 0.0 ? "Expires " + std::string(net::WeekdayName(net::DayOf(A->BonusExpires), true)) + ", " + net::DateLabel(net::DayOf(A->BonusExpires)) + " \xC2\xB7 " + net::Countdown(Left) + " left"
								   : std::string("Expired"),
					Big.X + Big.W - 30.0f, Big.Y + 244.0f, Ts(13.0f, 700, Left > 0.0 && Left < 7.0 * net::MinutesPerDay ? pal::Orange : pal::Muted, Align::Right));
				// What the next $5 takes: rake paid toward the next $20.
				UI.Text("$5 for every $20 of rake \xC2\xB7 " + Money(A->RakeLife) + " of rake paid here so far", Lx, Big.Y + 274.0f, Ts(13.0f, 600, pal::Muted));
			}
		}
		else
		{
			UI.Text(SitesBonusStart, Lx, Big.Y + 244.0f, Ts(16.0f, 700, pal::Ink));
		}
	}
	else
	{
		// RunItTwice: the welcome tickets and the Second Run Club side by side.
		const float Half = (Big.W - 90.0f) / 2.0f;
		NetSpaced(*C, "WELCOME TICKETS", Lx, Big.Y + 40.0f, 11.0f, 800, pal::Accent, 1.6f);
		UI.Text("$33 in tickets", Lx, Big.Y + 80.0f, Ts(26.0f, 900, pal::Ink));
		NetParagraph(*C, SitesTicketsRule, Lx, Big.Y + 110.0f, Half, 15.0f, 500, SitesTone(0xc3cedf), 21.0f, 3);
		float Ty = Big.Y + 190.0f;
		int Held = 0;
		const std::string Prefix = std::string(net::KeyOf(Site)) + ".";
		for (const auto& Tk : S.Life.Tickets)
		{
			if (Tk.second <= 0 || Tk.first.compare(0, Prefix.size(), Prefix) != 0 || Ty > Big.Y + Big.H - 30.0f)
			{
				continue;
			}
			const net::EventTemplate* T = net::Site(Site).FindTemplate(Tk.first);
			const std::string What = T ? NetShortName(T->Name) : Tk.first.substr(Prefix.size());
			NetPill(*C, "TICKET", Lx, Ty - 13.0f, pal::Gold, true, 9.0f);
			UI.Text(What + (Tk.second > 1 ? " \xC3\x97" + std::to_string(Tk.second) : std::string()), Lx + 64.0f, Ty, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, Half - 70.0f));
			Ty += 30.0f;
			++Held;
		}
		if (Held == 0 && A && A->Deposited > 0 && S.Life.Sites.Done.count("tickets:runittwice") == 0)
		{
			// Only a first deposit of $50 or more brings them (core-sites marks the grant "tickets:runittwice"): one under it
			// had its chance.
			NetParagraph(*C, SitesTicketsMissed, Lx, Ty, Half, 14.0f, 600, pal::Muted, 19.0f, 2);
		}
		else if (Held == 0)
		{
			UI.Text(SitesTicketsNone, Lx, Ty, Ts(14.0f, 600, pal::Muted));
		}
		// The club.
		const float Cx = Lx + Half + 30.0f;
		C->FillRect({Cx - 15.0f, Big.Y + 24.0f, 1.0f, Big.H - 48.0f}, pal::Line);
		NetSpaced(*C, "SECOND RUN CLUB", Cx, Big.Y + 40.0f, 11.0f, 800, pal::Accent, 1.6f);
		const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
		const Chips Rake = A && A->MonthKey == site::MonthKeyAt(World) ? A->RakeMonth : 0;
		const int Tier = site::TierFor(Club, Rake);
		const int Top = static_cast<int>(Club.size()) - 1;
		UI.Text(Tier > 0 ? SitesPct(Club[static_cast<size_t>(Tier)].Permille) + " back this month" : std::string("Not in it yet this month"), Cx, Big.Y + 80.0f, Ts(26.0f, 900, Tier > 0 ? pal::Gold : pal::Ink));
		NetParagraph(*C, SitesClubRule, Cx, Big.Y + 110.0f, Half, 15.0f, 500, SitesTone(0xc3cedf), 21.0f, 2);
		for (int I = 1; I <= Top && I < 5; ++I)
		{
			const site::TierInfo& Ti = Club[static_cast<size_t>(I)];
			const float Ry = Big.Y + 166.0f + Nf(I - 1) * 30.0f;
			const bool Here = I == Tier;
			if (Here)
			{
				UI.RRect({Cx - 8.0f, Ry - 20.0f, Half + 8.0f, 28.0f}, 8.0f, NetA(pal::Accent, 0.12f), NetA(pal::Accent, 0.5f));
			}
			UI.Text(std::string(Ti.Name) + " \xC2\xB7 from " + NetMoney(Ti.From) + " of rake", Cx, Ry, Ts(14.0f, 700, Here ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, false, Half - 60.0f));
			UI.Text(SitesPct(Ti.Permille), Cx + Half - 8.0f, Ry, Ts(14.0f, 800, Here ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		}
		std::string Line = Money(Rake) + " of rake this month";
		if (Tier < Top)
		{
			const site::TierInfo& Next = Club[static_cast<size_t>(Tier + 1)];
			Line += " \xC2\xB7 " + Money(Next.From - Rake) + " more to " + SitesPct(Next.Permille);
		}
		UI.Text(Line, Cx, Big.Y + Big.H - 22.0f, Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Half));
	}

	// Under it: what the player has paid here, and the money moving.
	const Rect Low{24.0f, 522.0f, 1052.0f, 428.0f};
	Panel(Low);
	Section("YOUR GAME ON " + SitesUpperName(Site), Low.X + 26.0f, Low.Y + 38.0f, pal::Accent);
	const std::pair<std::string, std::string> Facts[4] = {
		{A ? std::to_string(A->LifetimeEvents) : std::string("0"), "TOURNAMENTS HERE"},
		{A ? Money(A->MonthKey == site::MonthKeyAt(World) ? A->RakeMonth : 0) : Money(0), "RAKE THIS MONTH"},
		{A ? Money(A->RakeLife) : Money(0), "RAKE ALL TIME"},
		{A ? Money(A->LifetimeWon) : Money(0), "WON HERE"},
	};
	const float Fw = (Low.W - 52.0f) / 4.0f;
	for (int I = 0; I < 4; ++I)
	{
		const float X = Low.X + 26.0f + Nf(I) * Fw;
		UI.Text(Facts[I].first, X, Low.Y + 96.0f, Ts(26.0f, 900, I == 3 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, true));
		NetSpaced(*C, Facts[I].second, X, Low.Y + 120.0f, 9.5f, 800, pal::Muted, 1.2f);
	}
	NetParagraph(*C, SitesStaysThere, Low.X + 26.0f, Low.Y + 170.0f, Low.W - 52.0f, 15.0f, 600, SitesTone(0xc3cedf), 21.0f, 2);
	ButtonOpts O;
	O.Kind = ButtonKind::Primary;
	O.Size = 16.0f;
	if (SitesButton(UI, *C, "promocashier", {Low.X + 26.0f, Low.Y + Low.H - 74.0f, 220.0f, 48.0f}, "Open the Cashier", O))
	{
		OpenPage(Page::Cashier, Now);
	}

	// The site's letters.
	const Rect Rt{1092.0f, 84.0f, 484.0f, 866.0f};
	Panel(Rt);
	Section("FROM " + SitesUpperName(Site), Rt.X + 26.0f, Rt.Y + 40.0f, pal::Accent);
	const std::vector<const life::Letter*> Letters = SitesLetters(S, Site);
	if (Letters.empty())
	{
		NetParagraph(*C, SitesNoLetters, Rt.X + 26.0f, Rt.Y + 76.0f, Rt.W - 52.0f, 14.0f, 500, pal::Muted, 20.0f, 3);
	}
	float Y = Rt.Y + 60.0f;
	for (size_t I = 0; I < Letters.size() && I < 9; ++I)
	{
		const life::Letter& M = *Letters[I];
		const Rect Row{Rt.X + 14.0f, Y, Rt.W - 28.0f, 82.0f};
		const Ui::ClickState Rs = UI.Clickable("promoletter" + std::to_string(I), Row);
		if (Rs.Hover)
		{
			UI.RRect(Row, 10.0f, Rgba(255, 255, 255, 0.04f));
		}
		UI.Text(M.Subject, Row.X + 12.0f, Row.Y + 26.0f, Ts(15.0f, M.Read ? 700 : 900, pal::Ink, Align::Left, Baseline::Alphabetic, false, Row.W - 24.0f));
		UI.Text(M.From + " \xC2\xB7 " + net::DateLabel(net::DayOf(M.At)), Row.X + 12.0f, Row.Y + 46.0f, Ts(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Row.W - 24.0f));
		NetParagraph(*C, M.Body, Row.X + 12.0f, Row.Y + 66.0f, Row.W - 24.0f, 12.5f, 500, pal::Dim, 16.0f, 1);
		if (Rs.Clicked)
		{
			OpenApp(App::Mail, Now);
		}
		Y += 88.0f;
	}
}

// ------------------------------------------------------------------ a rival's Cashier

void RiverLine::SiteCashier(double Now)
{
	const net::SiteId Site = SitesClient(*this, S);
	const site::Account* A = SitesAccount(S, Site);
	SitesUi& U = SitesOf(this);
	const std::string Name = SitesShortName(Site);
	const net::CashMethod* In = SitesDepositMethod(Site);
	const net::CashMethod* Out = SitesWithdrawMethod(Site);
	const Chips Balance = A ? A->Cents : 0;
	const float Ease = NetEase((Now - PageAt) / 0.6);
	auto Answer = [&](const std::string& Why, const std::string& Done) {
		U.Note = Why.empty() ? Done : Why;
		U.NoteBad = !Why.empty();
		U.NoteAt = Now;
	};

	// The balance and what's on its way.
	const Rect Head{24.0f, 84.0f, 1052.0f, 196.0f};
	C->GlowRoundRect({Head.X, Head.Y + 8.0f, Head.W, Head.H}, 16.0f, Rgba(0, 0, 0, 0.4f), 20.0f);
	UI.RRect(Head, 16.0f, Paint::Linear({Head.X, 0.0f}, {Head.X + Head.W, 0.0f}, Mix(pal::Panel, pal::Accent, 0.18f), pal::Panel), NetA(pal::Accent, 0.35f));
	siteart::Logo(*C, {Head.X + 30.0f, Head.Y + 30.0f, 72.0f, 72.0f}, Site);
	NetSpaced(*C, "CASHIER", Head.X + 126.0f, Head.Y + 44.0f, 11.0f, 800, pal::Accent, 1.8f);
	UI.Text("Your balance on " + Name, Head.X + 126.0f, Head.Y + 72.0f, Ts(16.0f, 600, pal::Muted));
	UI.Text(Money(static_cast<Chips>(static_cast<double>(Balance) * static_cast<double>(Ease))), Head.X + 126.0f, Head.Y + 128.0f, Ts(50.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true));
	NetParagraph(*C, SitesStaysThere, Head.X + 126.0f, Head.Y + 162.0f, 560.0f, 13.5f, 600, pal::Muted, 19.0f, 2);
	{
		const Chips Coming = SitesInTransit(S, Site);
		const float Rx = Head.X + Head.W - 30.0f;
		NetSpaced(*C, "IN TRANSIT", Rx, Head.Y + 44.0f, 10.5f, 800, pal::Muted, 1.6f, Align::Right);
		UI.Text(Coming > 0 ? Money(Coming) : std::string("\xE2\x80\x94"), Rx, Head.Y + 92.0f, Ts(30.0f, 900, Coming > 0 ? pal::Ink : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		UI.Text(Coming > 0 ? std::string("arriving at your bank") : std::string(SitesNothingComing), Rx, Head.Y + 116.0f, Ts(13.0f, 600, pal::Muted, Align::Right));
		UI.Text("Bank " + Money(S.BankrollCents), Rx, Head.Y + 160.0f, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	}

	// Deposit.
	const Rect Dp{24.0f, 296.0f, 518.0f, 436.0f};
	Panel(Dp);
	UI.Text("Deposit", Dp.X + 26.0f, Dp.Y + 46.0f, Ts(24.0f, 900, pal::Ink));
	UI.Text(SitesDepositHow, Dp.X + 26.0f, Dp.Y + 72.0f, Ts(13.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Dp.W - 52.0f));
	U.Deposit = std::min(std::max(U.Deposit, 0), 3);
	for (int K = 0; K < 4; ++K)
	{
		const float Cw = (Dp.W - 52.0f - 3.0f * 10.0f) / 4.0f;
		const Rect Ch{Dp.X + 26.0f + Nf(K) * (Cw + 10.0f), Dp.Y + 94.0f, Cw, 42.0f};
		const Ui::ClickState St = UI.Clickable("cashin" + std::to_string(K), Ch);
		if (St.Clicked)
		{
			U.Deposit = K;
		}
		const bool On = U.Deposit == K;
		UI.RRect(Ch, 10.0f, On ? NetA(pal::Accent, 0.2f) : St.Hover ? SitesTone(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? pal::Accent : pal::Line, On ? 1.5f : 1.0f);
		UI.Text(NetMoney(SitesDepositSteps[K]), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, Ts(17.0f, 800, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Alphabetic, true));
	}
	const Chips Amount = SitesDepositSteps[U.Deposit];
	const Chips Fee = In ? SitesDepositFee(*In, Amount) : 0;
	const std::pair<std::string, std::string> DepRows[4] = {
		{"Lands on " + Name, Money(Amount)},
		{In ? In->Name + " fee (" + Fixed(static_cast<double>(In->FeePermille) / 10.0, In->FeePermille % 10 == 0 ? 0 : 1) + "%)" : std::string("Fee"), Money(Fee)},
		{"The bank pays", Money(Amount + Fee)},
		{"Bank after", Money(S.BankrollCents - Amount - Fee)},
	};
	for (int I = 0; I < 4; ++I)
	{
		const float Ry = Dp.Y + 176.0f + Nf(I) * 32.0f;
		UI.Text(DepRows[I].first, Dp.X + 26.0f, Ry, Ts(14.5f, 600, I == 2 ? pal::Ink : pal::Muted));
		UI.Text(DepRows[I].second, Dp.X + Dp.W - 26.0f, Ry, Ts(15.0f, 800, I == 0 ? pal::Gold : I == 3 && S.BankrollCents - Amount - Fee < 0 ? pal::Red : pal::Ink, Align::Right, Baseline::Alphabetic, true));
	}
	{
		std::string Why;
		if (!In)
		{
			Why = "No way in yet";
		}
		else if (In->MinCents > 0 && Amount < In->MinCents)
		{
			Why = "Minimum " + NetMoney(In->MinCents);
		}
		else if (S.BankrollCents < Amount + Fee)
		{
			Why = "Short " + Money(Amount + Fee - S.BankrollCents) + " in the bank";
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Primary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !Previewing;
		O.Sub = Why.empty() ? (In ? In->Name + " \xC2\xB7 lands at once" : std::string()) : Why;
		if (SitesButton(UI, *C, "cashdeposit", {Dp.X + 26.0f, Dp.Y + 314.0f, Dp.W - 52.0f, 60.0f}, "Deposit " + NetMoney(Amount), O))
		{
			Answer(S.Deposit(Site, Amount, In->Id), "Deposited " + Money(Amount) + ". It's on " + Name + " now.");
		}
	}
	NetParagraph(*C, SitesOnlyBank, Dp.X + 26.0f, Dp.Y + 404.0f, Dp.W - 52.0f, 12.5f, 600, pal::Dim, 17.0f, 2);

	// Withdraw.
	const Rect Wd{558.0f, 296.0f, 518.0f, 436.0f};
	Panel(Wd);
	UI.Text("Withdraw", Wd.X + 26.0f, Wd.Y + 46.0f, Ts(24.0f, 900, pal::Ink));
	UI.Text(SitesWithdrawHow, Wd.X + 26.0f, Wd.Y + 72.0f, Ts(13.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Wd.W - 52.0f));
	if (U.Withdraw <= 0)
	{
		U.Withdraw = SitesWithdrawSteps[0];
	}
	for (int K = 0; K < 5; ++K)
	{
		const float Cw = (Wd.W - 52.0f - 4.0f * 10.0f) / 5.0f;
		const Rect Ch{Wd.X + 26.0f + Nf(K) * (Cw + 10.0f), Wd.Y + 94.0f, Cw, 42.0f};
		const Chips Step = K < 4 ? SitesWithdrawSteps[K] : Balance;
		const Ui::ClickState St = UI.Clickable("cashout" + std::to_string(K), Ch);
		if (St.Clicked)
		{
			U.Withdraw = Step;
		}
		const bool On = U.Withdraw == Step && (K < 4 || Balance > 0);
		UI.RRect(Ch, 10.0f, On ? NetA(pal::Accent, 0.2f) : St.Hover ? SitesTone(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? pal::Accent : pal::Line, On ? 1.5f : 1.0f);
		UI.Text(K < 4 ? NetMoney(Step) : std::string("All"), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, Ts(16.0f, 800, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Alphabetic, true));
	}
	const Chips Take = U.Withdraw;
	const Chips OutFee = Out ? Out->WithdrawFeeCents : 0;
	const std::string When = Out ? SitesWindow(World, Out->WithdrawHoursMin, Out->WithdrawHoursMax) : std::string("\xE2\x80\x94");
	const std::pair<std::string, std::string> OutRows[4] = {
		{"Leaves " + Name + " now", Money(Take)},
		{Out ? Out->Name + " fee" : std::string("Fee"), Money(OutFee)},
		{"Reaches your bank", Money(std::max<Chips>(0, Take - OutFee))},
		{"Expected", When},
	};
	for (int I = 0; I < 4; ++I)
	{
		const float Ry = Wd.Y + 176.0f + Nf(I) * 32.0f;
		UI.Text(OutRows[I].first, Wd.X + 26.0f, Ry, Ts(14.5f, 600, I == 2 ? pal::Ink : pal::Muted));
		UI.Text(OutRows[I].second, Wd.X + Wd.W - 26.0f, Ry, Ts(15.0f, 800, I == 2 ? pal::Gold : pal::Ink, Align::Right, Baseline::Alphabetic, I < 3));
	}
	{
		std::string Why;
		// The site's own floor for a withdrawal (a method's MinCents is its deposit side's).
		const Chips Min = SitesRegistered(Site) ? net::Def(Site).MinWithdraw : 0;
		if (!Out)
		{
			Why = "No way out yet";
		}
		else if (Take <= 0 || Take > Balance)
		{
			Why = "Balance " + Money(Balance);
		}
		else if (Min > 0 && Take < Min)
		{
			Why = "Minimum " + NetMoney(Min);
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !Previewing;
		O.Sub = Why.empty() ? "Expected " + When : Why;
		if (SitesButton(UI, *C, "cashwithdraw", {Wd.X + 26.0f, Wd.Y + 314.0f, Wd.W - 52.0f, 60.0f}, "Withdraw " + Money(Take), O))
		{
			Answer(S.Withdraw(Site, Take, Out->Id), "Requested. " + Money(std::max<Chips>(0, Take - OutFee)) + " is on its way to your bank.");
			U.Withdraw = SitesWithdrawSteps[0];
		}
	}
	if (!U.Note.empty() && Now - U.NoteAt < 6.0)
	{
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Nf(std::min(1.0, (6.0 - (Now - U.NoteAt)) / 0.6)));
		NetParagraph(*C, U.Note, Wd.X + 26.0f, Wd.Y + 404.0f, Wd.W - 52.0f, 13.0f, 700, U.NoteBad ? pal::Red : pal::Green, 17.0f, 2);
		C->SetAlpha(A0);
	}

	// Money on its way back.
	const Rect Tr{24.0f, 748.0f, 1052.0f, 202.0f};
	Panel(Tr);
	Section("ON ITS WAY TO YOUR BANK", Tr.X + 26.0f, Tr.Y + 36.0f, pal::Accent);
	float Ty = Tr.Y + 70.0f;
	int Shown = 0;
	for (const site::Transfer& T : S.Life.Sites.Transfers)
	{
		if (T.Site != Site || !T.Out || T.State != 0 || Shown >= 3)
		{
			continue;
		}
		const net::CashMethod* M = SitesWithdrawMethod(Site);
		const bool Range = M && M->WithdrawHoursMax > M->WithdrawHoursMin;
		const std::string Due = Range ? SitesWindow(T.RequestedAt, M->WithdrawHoursMin, M->WithdrawHoursMax) : std::string(net::WeekdayName(net::DayOf(T.ArrivesAt))) + " " + net::TimeLabel(T.ArrivesAt);
		C->FillCircle(Tr.X + 34.0f, Ty - 5.0f, 4.0f, pal::Accent2);
		UI.Text("Withdrawal of " + Money(T.Cents) + " \xC2\xB7 requested " + net::WeekdayName(net::DayOf(T.RequestedAt)) + " " + net::TimeLabel(T.RequestedAt), Tr.X + 48.0f, Ty, Ts(14.5f, 700, pal::Ink));
		UI.Text(Money(std::max<Chips>(0, T.Cents - T.FeeCents)) + " arriving " + Due, Tr.X + Tr.W - 26.0f, Ty, Ts(14.5f, 800, pal::Accent2, Align::Right));
		Ty += 36.0f;
		++Shown;
	}
	if (Shown == 0)
	{
		UI.Text(SitesNothingComing, Tr.X + 26.0f, Ty, Ts(15.0f, 600, pal::Muted));
	}

	// The site's own lines (its ledger, the last 40).
	const Rect Rt{1092.0f, 84.0f, 484.0f, 866.0f};
	Panel(Rt);
	Section("ACTIVITY ON " + SitesUpperName(Site), Rt.X + 26.0f, Rt.Y + 40.0f, pal::Accent);
	if (!A || A->Lines.empty())
	{
		NetParagraph(*C, SitesNoLines, Rt.X + 26.0f, Rt.Y + 78.0f, Rt.W - 52.0f, 14.0f, 500, pal::Muted, 20.0f, 3);
		return;
	}
	float Ly = Rt.Y + 64.0f;
	int Row = 0;
	for (auto It = A->Lines.rbegin(); It != A->Lines.rend() && Ly + 46.0f <= Rt.Y + Rt.H - 10.0f; ++It, ++Row)
	{
		const site::Line& L = *It;
		const float Ri = NetEase((Now - PageAt - 0.03 * static_cast<double>(Row)) / 0.4);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		UI.Text(L.Label, Rt.X + 26.0f, Ly + 20.0f, Ts(14.5f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, Rt.W - 180.0f));
		UI.Text(std::string(net::WeekdayName(net::DayOf(L.At))) + " " + net::TimeLabel(L.At), Rt.X + 26.0f, Ly + 38.0f, Ts(12.0f, 500, pal::Muted));
		UI.Text(L.Cents == 0 ? std::string("\xE2\x80\x94") : (L.Cents > 0 ? "+" : "\xE2\x88\x92") + Money(L.Cents > 0 ? L.Cents : -L.Cents), Rt.X + Rt.W - 26.0f, Ly + 28.0f,
			Ts(15.0f, 800, L.Cents > 0 ? pal::Green : L.Cents < 0 ? pal::Ink : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		C->FillRect({Rt.X + 26.0f, Ly + 48.0f, Rt.W - 52.0f, 1.0f}, NetA(pal::Line, 0.6f));
		C->SetAlpha(A0);
		Ly += 52.0f;
	}
}

namespace rlsites_detail
{
std::vector<std::string> SitesUiInventory()
{
	return {SitesStaysThere, SitesNothingComing, SitesDepositHow, SitesWithdrawHow, SitesOnlyBank, SitesNoLines, SitesBonusRule, SitesBonusStart, SitesBonusDone, SitesTicketsRule,
		SitesTicketsNone, SitesClubRule, SitesNoLetters, SitesMenuNote, SitesInviteOnce, SitesInviteLast, SitesInviteDeclined, SitesPublishArmed, SitesAboutMonth, SitesAboutSeason, SitesAboutWins, SitesAboutSeries, SitesAboutAny,
		SitesBonusSlide, SitesClubSlide, SitesOwnMoney, SitesStrikeConfirm, SitesKastClient, SitesAboutSeasonYear, SitesBonusClosed, SitesInviteDone, SitesTicketsMissed};
}
} // namespace rlsites_detail
} // namespace ui
} // namespace ss
