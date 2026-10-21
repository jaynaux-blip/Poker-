// Update 3 (SPEC §2.2-§2.5): Ignite Poker, the Arcade on the Boardwalk, ui-ignite's file: the client frame (the marquee, the
// Boardwalk band, the dock), PLAY, the event sheet, BOARDS, BUZZ, WALLET, the Player Card, "Your run" and the capsule
// table's layer. G1 (§2.9): the look on today's data. Every sign is live (the next Flip & Run on the Fuse Booth, tonight's
// starts on the wheel, the welcome bonus on the Prize Counter), the centre button reads FLIP, and nothing says it is coming:
// the loops (Sparks, the Flame, Dares, the wheel's rings, FUSE) arrive with G2's View.
//
// The gatekeepers stay single-sourced: the shelves are ScheduleRows' rows (the same Joinable and lock) written to Listed in
// shelf-then-tile order, a registration goes through RegisterBlock (the clause's two presses, the strike gate's token), the
// money through Session::Deposit and Withdraw, the table through the shared Table() in Ignite's SiteLook (ui-tablestyle's).
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/SitesDetail.h"
#include "IgniteArt.h"
#include "RiverLineShared.h"

#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace ss
{
namespace ui
{
namespace rlignite_detail
{
// ------------------------------------------------------------------ the laptop's side state

std::map<const RiverLine*, IgniteUi>& IgniteStates()
{
	static std::map<const RiverLine*, IgniteUi> ByLaptop;
	return ByLaptop;
}

IgniteUi& IgniteOf(const RiverLine* RL)
{
	return IgniteStates()[RL];
}

void IgniteForget(const RiverLine* RL)
{
	IgniteStates().erase(RL);
}

bool IgniteSheetOpen(const RiverLine* RL)
{
	const auto It = IgniteStates().find(RL);
	return It != IgniteStates().end() && It->second.Sheet;
}

void IgniteOpenSheet(RiverLine& RL, const std::string& EventId, double Now)
{
	IgniteUi& U = IgniteOf(&RL);
	if (!U.Sheet || Access::EventId(RL) != EventId)
	{
		U.SheetAt = Now;
		U.SheetTab = 0;
	}
	Access::SelectEvent(RL, EventId);
	U.Sheet = true;
	U.FollowId = EventId;
}

void IgniteCue(Session& S, IgniteUi& U, const std::string& Key, SoundId Id, float Vol, bool Preview)
{
	if (Preview || U.Played.count(Key) > 0)
	{
		return;
	}
	if (U.Played.size() > 400)
	{
		U.Played.clear();
	}
	U.Played.insert(Key);
	S.UiSound(Id, Vol);
}

/** What one frame of Ignite's client works with. */
struct IgniteCtx
{
	RiverLine& RL;
	Session& S;
	Canvas& C;
	Ui& UI;
	IgniteUi& U;
	igniteart::IgniteTones K;
	double Now = 0.0;   // the real clock (input, entrances)
	double Tm = 0.0;    // the presentation clock (a settled still while Previewing)
	double World = 0.0; // world minutes
	double Dt = 0.0;
	bool Preview = false;
	net::SiteId Site = net::SiteId::Ignite;
};

/** A settled clock for the stream preview's stills (every pulse at rest, every entrance done). */
constexpr double IgniteStill = 1000.0;

float IgniteEase(double T)
{
	return static_cast<float>(EaseOutCubic(Clamp01(T)));
}

float IgniteFollow(double Dt, double Rate)
{
	return static_cast<float>(1.0 - std::exp(-Dt * Rate));
}

/** Ease-out-back from 0.85 (SPEC §2.3: entrances). */
float IgniteBack(double T)
{
	const double U = Clamp01(T);
	const double C1 = 1.70158;
	const double C3 = C1 + 1.0;
	return static_cast<float>(1.0 + C3 * std::pow(U - 1.0, 3.0) + C1 * std::pow(U - 1.0, 2.0));
}

/** A breathing pulse on the beat (0..1). */
float IgnitePulse(const IgniteCtx& X)
{
	const double B = motion::Beat(X.Tm, X.K.Bpm);
	return static_cast<float>(0.5 + 0.5 * std::cos(B * 2.0 * 3.14159265358979));
}

Color IgniteHx(uint32_t Rgb, float A = 1.0f)
{
	return Hex(Rgb, A);
}

std::string IgniteUpper(const std::string& S)
{
	return rlnet_detail::NetUpper(S);
}

/** "1D 23H", "4H 12M", "12M": a sign's countdown in caps. */
std::string IgniteSignTime(double Minutes)
{
	return IgniteUpper(net::Countdown(std::max(0.0, Minutes)));
}

/** A start on the hour without its minutes ("8 PM"), with its weekday when it isn't today. */
std::string IgniteWhen(double Start, double World)
{
	const std::string T = rlnet_detail::LiveTime(Start);
	return net::DayOf(Start) == net::DayOf(World) ? T : std::string(net::WeekdayName(net::DayOf(Start))) + " " + T;
}

/** Whether a start is tonight's: today's by the calendar, or before the coming 6 AM (a night runs on past midnight). A sign
 * or a shout that says "tonight" says it only of these. */
bool IgniteTonight(double Start, double World)
{
	const double NextSix = std::floor((World - 6.0 * 60.0) / net::MinutesPerDay) * net::MinutesPerDay + 30.0 * 60.0;
	return net::DayOf(Start) == net::DayOf(World) || Start < NextSix;
}

const site::Account* IgniteAccount(const Session& S)
{
	return rlsites_detail::SitesAccount(S, net::SiteId::Ignite);
}

/**
 * The open (or running) instance of a template's family nearest from now, or false. A family is the template and its
 * launch-week one-offs ("ignite.sunday-blaze-launch3" stands in for the daily on its day).
 */
bool IgniteNextOf(const net::Network& Net, const std::string& TemplateId, double World, net::EventInstance& Out)
{
	// A day at a time from today, stopping at the first (a week's schedule is a few hundred instances).
	for (int Day = 0; Day <= 8; ++Day)
	{
		const double From = Day == 0 ? World - 8.0 * 60.0 : std::floor(World / net::MinutesPerDay + static_cast<double>(Day)) * net::MinutesPerDay;
		const double To = std::floor(World / net::MinutesPerDay + static_cast<double>(Day) + 1.0) * net::MinutesPerDay;
		for (const net::EventInstance& E : Net.Window(From, To))
		{
			const std::string& Id = Net.TemplateOf(E).Id;
			if (Id != TemplateId && Id.rfind(TemplateId + "-launch", 0) != 0)
			{
				continue;
			}
			const net::Status St = Net.Live(E, World).St;
			if (St == net::Status::Announced || St == net::Status::Registering || St == net::Status::LateReg || St == net::Status::Running || St == net::Status::FinalTable)
			{
				Out = E;
				return true;
			}
		}
	}
	return false;
}

/** Whether a status is one a featured start or the next Flip & Run is picked in (announced, registering, late reg). */
bool IgniteOpenish(net::Status St)
{
	return St == net::Status::Announced || St == net::Status::Registering || St == net::Status::LateReg;
}

/** The featured starts (registering, late reg or announced) in IgniteFeatured's order before its cut: soonest first, then
 * the featured ahead of a big guarantee (the headliners outrank it). */
std::vector<net::EventInstance> IgniteFeaturedOrder(const net::Network& Net, double World)
{
	std::vector<net::EventInstance> Out;
	for (const net::EventInstance& E : Net.Window(World - 4.0 * 60.0, World + 30.0 * 60.0))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		if ((T.Featured || T.GtdCents >= 200000) && IgniteOpenish(Net.Live(E, World).St))
		{
			Out.push_back(E);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const net::EventInstance& A, const net::EventInstance& B) { return A.Start < B.Start; });
	std::stable_sort(Out.begin(), Out.end(), [&Net](const net::EventInstance& A, const net::EventInstance& B) { return Net.TemplateOf(A).Featured && !Net.TemplateOf(B).Featured; });
	return Out;
}

/** The first Max of an ordered list of featured starts, soonest first. */
std::vector<net::EventInstance> IgniteFeaturedCut(const std::vector<net::EventInstance>& Order, size_t Max)
{
	std::vector<net::EventInstance> Out(Order.begin(), Order.begin() + static_cast<std::ptrdiff_t>(std::min(Max, Order.size())));
	std::stable_sort(Out.begin(), Out.end(), [](const net::EventInstance& A, const net::EventInstance& B) { return A.Start < B.Start; });
	return Out;
}

/** The next featured starts (registering, late reg or announced), soonest first. */
std::vector<net::EventInstance> IgniteFeatured(const net::Network& Net, double World, size_t Max)
{
	return IgniteFeaturedCut(IgniteFeaturedOrder(Net, World), Max);
}

/** The deal that strikes a table here, as one line ("Under your Team RiverLine deal, every table here is a strike."); "" if none. */
std::string IgniteClauseBanner(const Session& S)
{
	const std::string Warn = rlsites_detail::SitesClauseWarning(S, net::SiteId::Ignite, false, false);
	if (Warn.empty())
	{
		return std::string();
	}
	const std::string Head = "Strike under your ";
	const std::string Tail = " deal";
	std::string Team = "sponsor's";
	if (Warn.compare(0, Head.size(), Head) == 0 && Warn.size() > Head.size() + Tail.size() && Warn.compare(Warn.size() - Tail.size(), Tail.size(), Tail) == 0)
	{
		Team = Warn.substr(Head.size(), Warn.size() - Head.size() - Tail.size());
	}
	return rlsites_detail::SitesFill(IgniteClauseLine, {{"Team", Team}});
}

IgniteWelcome IgniteWelcomeOf(const Session& S, double World)
{
	IgniteWelcome W;
	const site::Account* A = IgniteAccount(S);
	if (!A)
	{
		return W;
	}
	const site::show::WelcomeView V = site::show::Welcome(*A, World);
	W.Pending = A->BonusPending;
	W.Released = A->BonusReleased;
	W.Started = A->BonusExpires > 0.0;
	W.Closed = W.Started && A->BonusPending == 0 && World >= A->BonusExpires;
	W.Done = W.Started && A->BonusPending == 0 && !W.Closed;
	W.Ends = A->BonusExpires;
	W.NextIn = V.NextIn;
	const Chips Total = W.Pending + W.Released;
	W.Needle = V.NeedleT > 0.0 ? V.NeedleT : Total > 0 ? static_cast<double>(W.Released) / static_cast<double>(Total) : 0.0;
	return W;
}

bool IgniteBlaze(const net::Network& Net, double World, net::EventInstance& Out);
bool IgniteNextFlip(const net::Network& Net, double World, net::EventInstance& Out);

/**
 * The schedule's picks this frame (G1_AUDIT M3): the featured order, the next Sunday Blaze and the next Flip & Run, scanned
 * once a world minute. A status changes on a whole minute, and a pick can only stop qualifying (an earlier start that
 * didn't qualify never comes back), so each frame checks the picks still standing against Live and scans again if one
 * fell: the same answers as a scan every frame, at a few Live calls.
 */
const IgniteUi::SchedulePicks& IgnitePicksOf(IgniteCtx& X)
{
	IgniteUi::SchedulePicks& P = X.U.Picks;
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	const long long Minute = static_cast<long long>(std::floor(X.World));
	bool Fresh = P.Minute == Minute;
	for (size_t I = 0; Fresh && I < P.Featured.size() && I < 3; ++I)
	{
		Fresh = IgniteOpenish(Net.Live(P.Featured[I], X.World).St);
	}
	if (Fresh && P.HasBlaze)
	{
		const net::Status St = Net.Live(P.Blaze, X.World).St;
		Fresh = IgniteOpenish(St) || St == net::Status::Running || St == net::Status::FinalTable;
	}
	if (Fresh && P.HasFlip)
	{
		Fresh = IgniteOpenish(Net.Live(P.Flip, X.World).St);
	}
	if (!Fresh)
	{
		P.Minute = Minute;
		P.Featured = IgniteFeaturedOrder(Net, X.World);
		P.HasBlaze = IgniteBlaze(Net, X.World, P.Blaze);
		P.HasFlip = IgniteNextFlip(Net, X.World, P.Flip);
	}
	return P;
}

/** IgniteFeatured from this minute's picks. */
std::vector<net::EventInstance> IgniteFeaturedNow(IgniteCtx& X, size_t Max)
{
	return IgniteFeaturedCut(IgnitePicksOf(X).Featured, Max);
}

/** The next Sunday Blaze from this minute's picks. */
bool IgniteBlazeNow(IgniteCtx& X, net::EventInstance& Out)
{
	const IgniteUi::SchedulePicks& P = IgnitePicksOf(X);
	if (P.HasBlaze)
	{
		Out = P.Blaze;
	}
	return P.HasBlaze;
}

// ------------------------------------------------------------------ the marquee (SPEC §2.4, chrome)

void IgniteMarquee(IgniteCtx& X)
{
	Canvas& C = X.C;
	Ui& UI = X.UI;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	C.FillRect({0.0f, 0.0f, 1600.0f, IgniteMarqueeH}, IgniteHx(IgNight));
	C.FillRect({0.0f, 0.0f, 1600.0f, IgniteMarqueeH}, Paint::Linear({0.0f, 0.0f}, {1600.0f, 0.0f}, IgniteHx(K.BandA, 0.24f), IgniteHx(K.BandB, 0.24f)));
	C.FillRect({0.0f, 0.0f, 1600.0f, 30.0f}, Rgba(255, 255, 255, 0.03f));
	C.FillRect({0.0f, 63.0f, 1600.0f, 1.0f}, Rgba(0, 0, 0, 0.35f));
	// The marquee's still parts replay from the cache (G1_AUDIT M3: the marquee sits over every page and table); the clocks,
	// the crowd and the flame draw live. A piece that never moves is cached where it sits (drawn in place, replayed at the
	// origin), so it lands on exactly the floats it always did.
	const uint64_t Tone = (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u) ^ (Skin::Current().Key() << 4);
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:marquee-logo")) << 16) ^ Tone, 0.0f, 0.0f, [](Canvas& At) { siteart::Logo(At, {24.0f, 16.0f, 190.0f, 40.0f}, net::SiteId::Ignite); });
	const net::Network& Net = net::Site(net::SiteId::Ignite);

	// G1 (SPEC §2.4): where Sparks will count up, the next featured event's countdown (a click opens its sheet).
	{
		const Rect R{230.0f, 18.0f, 170.0f, 36.0f};
		const std::vector<net::EventInstance> Next = IgniteFeaturedNow(X, 1);
		const Ui::ClickState St = UI.Clickable("igspk", R, !Next.empty() && !X.Preview);
		const bool Hot = St.Hover;
		const std::string Name = Next.empty() ? std::string() : IgniteUpper(Net.TemplateOf(Next.front()).Name);
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:igspk|" + Name + (Hot ? "|hot" : ""))) << 16) ^ Tone, 0.0f, 0.0f, [&R, &Name, Hot](Canvas& At) {
			IgnitePill(At, R, IgniteHx(0x1b2610, Hot ? 1.0f : 0.92f), IgniteHx(IgLime, 0.55f), 1.5f);
			IgniteSpark(At, 250.0f, 36.0f, 10.0f, Hex(IgLime));
			if (!Name.empty())
			{
				At.Text(Name, 266.0f, 31.0f, IgniteTx(8.5f, 800, IgniteHx(IgLime, 0.75f), Align::Left, Baseline::Alphabetic, 126.0f));
			}
			else
			{
				At.Text("TONIGHT", 266.0f, 43.0f, IgniteBi(15.0f, Hex(IgLime)));
			}
		});
		if (!Next.empty())
		{
			const net::LiveState L = Net.Live(Next.front(), X.World);
			const std::string Clock = L.St == net::Status::LateReg ? "LATE " + rlnet_detail::NetHms(L.LateRegLeft) : rlnet_detail::NetHms(L.St == net::Status::Announced ? Next.front().Start - X.World : L.StartsIn);
			C.Text(Clock, 266.0f, 48.0f, IgniteBi(15.0f, Hex(IgLime), Align::Left, Baseline::Alphabetic, 126.0f));
			if (St.Clicked)
			{
				IgniteOpenSheet(X.RL, Next.front().Id, X.Now);
				Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
				X.S.ShowLobby();
			}
		}
	}
	// G1: where the Flame will burn, the crowd on the boardwalk right now (the site's own traffic, as its ticker counts it).
	{
		const Rect R{412.0f, 18.0f, 120.0f, 36.0f};
		const net::TrafficRule& Tr = net::Def(net::SiteId::Ignite).Traffic;
		const double Hour = std::fmod(X.World, net::MinutesPerDay) / 60.0;
		// The ticker's own figure (RiverLine::Ticker: the site's swing and its wobble), so the two never disagree.
		const double Online = static_cast<double>(Tr.OnlineBase) + static_cast<double>(Tr.OnlineSwing) * std::cos((Hour - static_cast<double>(Tr.PeakHour)) / 24.0 * 2.0 * 3.14159265) +
			900.0 * Tr.Scale * std::sin(X.World * 0.7);
		C.Cached(static_cast<uint64_t>(Fnv1a("ignite:online-pill")) << 16, 0.0f, 0.0f, [&R](Canvas& At) {
			IgnitePill(At, R, IgniteHx(0x2b140f), IgniteHx(IgTang, 0.55f), 1.5f);
			At.Text("ONLINE", 452.0f, 49.0f, IgniteTx(8.5f, 800, IgniteHx(IgTang, 0.85f)));
		});
		// The flame grows with the crowd, in quarter-unit steps that replay from the cache (G1_AUDIT M3).
		const float Size = std::round(4.0f * static_cast<float>(14.0 + 6.0 * Clamp01((Online - static_cast<double>(Tr.OnlineBase - Tr.OnlineSwing)) / std::max(1.0, 2.0 * static_cast<double>(Tr.OnlineSwing))))) / 4.0f;
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:crowd-flame")) << 32) ^ (static_cast<uint64_t>(std::lround(Size * 4.0f)) << 24) ^ static_cast<uint64_t>(K.Coral), 0.0f, 0.0f, [&K, Size](Canvas& At) {
			IgniteFlame(At, 434.0f, 49.0f, Size, Hex(K.Coral), Hex(IgTang), Hex(IgSun));
		});
		const std::string Count = Online >= 1000.0 ? Fixed(Online / 1000.0, Online >= 100000.0 ? 0 : 1) + "K" : Grouped(static_cast<int64_t>(Online));
		C.Text(Count, 452.0f, 37.0f, IgniteBi(15.0f, Hex(K.Cream)));
	}
	// The ring: the player's own bubble (the level's arc is G2); it opens the Player Card.
	{
		const Ui::ClickState St = UI.Clickable("lvring", {544.0f, 12.0f, 48.0f, 48.0f}, !X.Preview);
		const bool Hot = St.Hover;
		const std::string& Hero = X.S.HeroName;
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:lvring|" + Hero + (Hot ? "|hot" : ""))) << 16) ^ Tone, 0.0f, 0.0f, [&K, &Hero, Hot](Canvas& At) {
			At.FillCircle(568.0f, 36.0f, 24.0f, IgniteHx(0x1d1230));
			At.StrokeArc(568.0f, 36.0f, 22.0f, 0.0f, 2.0f * Pi, IgniteHx(Hot ? K.Coral : igniteart::IgViolet, Hot ? 1.0f : 0.7f), 3.0f);
			DrawAvatar(At, 568.0f, 36.0f, 17.5f, AvatarFor(Hero));
		});
		if (St.Clicked)
		{
			X.S.ShowLobby();
			Access::OpenPage(X.RL, RiverLine::Page::Profile, X.Now);
		}
	}
	// The table bubbles (the TableStrip's data): emblem, short name, stack in big blinds; a coral ring on "your turn".
	{
		const std::vector<rlsites_detail::StripRow> Rows = rlsites_detail::SitesTableStrip(X.RL, X.S);
		float Bx = 612.0f;
		for (size_t I = 0; I < Rows.size() && I < 4; ++I)
		{
			const rlsites_detail::StripRow& Row = Rows[I];
			const Rect R{Bx, 18.0f, 128.0f, 36.0f};
			const Ui::ClickState St = UI.Clickable("igt" + std::to_string(I), R, !X.Preview);
			const bool Front = X.S.CurrentScreen == Screen::Table && !X.S.Tiled && X.S.T && X.S.Glance(Row.Table).EventId == X.S.T->Spec.Id;
			if (Row.YourTurn)
			{
				const float P = IgnitePulse(X);
				C.GlowRoundRect({R.X - 4.0f, R.Y - 4.0f, R.W + 8.0f, R.H + 8.0f}, 22.0f, IgniteHx(K.Coral, 0.18f + 0.22f * P), 10.0f);
				IgnitePill(C, R, IgniteHx(0x2a1236), Hex(K.Coral), 2.5f);
			}
			else
			{
				IgnitePill(C, R, IgniteHx(St.Hover || Front ? K.Panel : K.Well), IgniteHx(Front ? K.Coral : K.Rim), Front ? 2.0f : 1.5f);
			}
			const net::Network& Tn = net::ForEvent(X.S.Glance(Row.Table).EventId);
			net::EventInstance Ei;
			if (Tn.FindInstance(X.S.Glance(Row.Table).EventId, Ei))
			{
				// The event's colour and a spark (the glyph would cost four bubbles a thousand vertices on every page).
				C.FillCircle(Bx + 18.0f, 36.0f, 13.0f, IgniteEventTint(Tn.TemplateOf(Ei), K));
				IgniteSpark(C, Bx + 18.0f, 36.0f, 7.5f, Hex(K.Cream));
			}
			C.Text(Row.Short, Bx + 36.0f, 33.0f, IgniteTx(11.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 86.0f));
			const std::string Bb = Fixed(Row.StackBb, Row.StackBb < 10.0 ? 1 : 0);
			const float Bw = C.Text(Bb, Bx + 36.0f, 49.0f, IgniteBi(13.0f, Row.YourTurn ? Hex(IgSun) : Hex(IgLime)));
			C.Text("bb", Bx + 40.0f + Bw, 49.0f, IgniteTx(10.0f, 700, pal::Muted));
			if (Row.YourTurn)
			{
				C.FillRoundRect({Bx + 70.0f, 4.0f, 56.0f, 15.0f}, 7.5f, Hex(K.Coral));
				C.Text("YOUR TURN", Bx + 98.0f, 15.0f, IgniteTx(8.0f, 900, Hex(0x2a0d14), Align::Center));
			}
			if (St.Clicked)
			{
				X.S.ShowTables();
				X.S.FocusTable(Row.Table);
			}
			Bx += 136.0f;
		}
	}
	// BUZZ: the bell with the unread count.
	{
		int Unread = 0;
		for (const net::NewsItem& It : Access::NewsFeed(X.RL))
		{
			Unread += It.At > Access::NewsSeenAt(X.RL) ? 1 : 0;
		}
		const Rect R{1150.0f, 18.0f, 48.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable("igbuzz", R, !X.Preview);
		const bool On = X.S.CurrentScreen == Screen::Lobby && Access::Shown(X.RL) == RiverLine::Page::News;
		const bool Lit = On || St.Hover;
		const int Shown = std::min(Unread, 10);
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:igbuzz")) << 16) ^ (static_cast<uint64_t>(Shown) << 8) ^ (Lit ? 0x40u : 0u) ^ (On ? 0x80u : 0u) ^ Tone, 0.0f, 0.0f, [&K, &R, Lit, On, Shown](Canvas& At) {
			IgnitePill(At, R, IgniteHx(Lit ? K.Panel : K.Well), IgniteHx(On ? K.Coral : K.Rim), 1.5f);
			IgniteBell(At, 1174.0f, 36.0f, 22.0f, Hex(K.Cream));
			if (Shown > 0)
			{
				At.FillCircle(1192.0f, 20.0f, 9.0f, Hex(IgPink));
				// The count as it is ("9+" past nine: a capped "9" would be a number that isn't true).
				At.Text(Shown > 9 ? std::string("9+") : std::to_string(Shown), 1192.0f, 20.5f, IgniteTx(Shown > 9 ? 9.5f : 11.0f, 900, Hex(IgDarkInk), Align::Center, Baseline::Middle));
			}
		});
		if (St.Clicked)
		{
			X.S.ShowLobby();
			Access::OpenPage(X.RL, RiverLine::Page::News, X.Now);
		}
	}
	// The balance (one run: the Cashier and results tests read it) and the clock.
	C.Text("Balance " + Money(rlsites_detail::SitesBalance(X.S, net::SiteId::Ignite)), 1460.0f, 43.0f, IgniteTx(15.0f, 700, pal::Gold, Align::Right));
	C.Text(ClockString(X.S.ClockMinutes()), 1576.0f, 37.0f, IgniteBi(18.0f, Hex(K.Cream), Align::Right));
	C.Text(IgniteUpper(std::string(net::WeekdayName(net::DayOf(X.World))) + " " + net::DateLabel(net::DayOf(X.World))), 1576.0f, 55.0f, IgniteTx(10.0f, 700, pal::Muted, Align::Right));
	// The bulbs: 66, two-tone, every third lit and stepping on the beat. For the four fifths of a beat after a step's rise the
	// row is one of three still patterns (motion::Chase: lit at exactly 1, the rest at 0.25), so those replay from the cache:
	// the marquee sits over every page and table, and drawing 66 bulbs afresh cost about a tenth of PLAY's draw. The rise
	// itself (a fifth of a beat, about three frames) comes up in four steps that replay too (G1_AUDIT M3).
	const double Beats = X.Tm * K.Bpm / 60.0;
	const double Into = Beats - std::floor(Beats);
	const int Phase = static_cast<int>(((static_cast<long long>(std::floor(Beats)) % 3) + 3) % 3);
	const int Rise = Into >= 0.2 ? 4 : std::min(3, static_cast<int>(Into / 0.2 * 4.0));
	const float Lit = Rise == 4 ? 1.0f : 0.25f + 0.75f * (static_cast<float>(Rise) + 0.5f) / 4.0f;
	const uint64_t Key = (static_cast<uint64_t>(Fnv1a("ignite:marquee-bulbs")) << 8) | static_cast<uint64_t>(Phase) | (Rise == 4 ? 0u : static_cast<uint64_t>(Rise + 1) << 4);
	C.Cached(Key, 0.0f, 0.0f, [Phase, Lit](Canvas& At) {
		for (int I = 0; I < 66; ++I)
		{
			IgniteBulb(At, 12.0f + static_cast<float>(I) * 24.0f, 69.0f, 3.0f, I % 2 == 0 ? Hex(IgSun) : Hex(0xffe7a8), Hex(IgPink), I % 3 == Phase && Lit > 0.3f ? Lit : 0.0f);
		}
	});
}

// ------------------------------------------------------------------ the dock (SPEC §2.4, chrome)

/** The next Sunday Blaze (open, running or to come). */
bool IgniteBlaze(const net::Network& Net, double World, net::EventInstance& Out)
{
	return IgniteNextOf(Net, "ignite.sunday-blaze", World, Out);
}

/** The Flip & Run whose seats are open now, or the next one. */
bool IgniteNextFlip(const net::Network& Net, double World, net::EventInstance& Out)
{
	for (const net::EventInstance& E : Net.Window(World - 60.0, World + 26.0 * 60.0))
	{
		const std::string& Id = Net.TemplateOf(E).Id;
		if (Net.TemplateOf(E).Fmt != net::Format::Flip || (Id != "ignite.flip-go" && Id.rfind("ignite.flip-go-launch", 0) != 0))
		{
			continue;
		}
		const net::Status St = Net.Live(E, World).St;
		if (St == net::Status::Registering || St == net::Status::LateReg || St == net::Status::Announced)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

void IgniteDock(IgniteCtx& X, bool FlipChip)
{
	Canvas& C = X.C;
	Ui& UI = X.UI;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const RiverLine::Page Pg = Access::Shown(X.RL);
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	// The dock's still parts replay from the cache, keyed by what they show (its pages, the pointer, the bonus's figures, the
	// hour's tones and the skin's muted ink); the breathing button and the Blaze's clock draw live (G1_AUDIT M3).
	const uint64_t Tone = (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u) ^ (Skin::Current().Key() << 4);
	// G1, left (SPEC §2.4, F11): where the Wristband will hang, the welcome bonus's Fuel Gauge; it opens REWARDS.
	{
		const Rect R{24.0f, 884.0f, 340.0f, 64.0f};
		const Ui::ClickState St = UI.Clickable("dockband", R, !X.Preview);
		const IgniteWelcome W = IgniteWelcomeOf(X.S, X.World);
		std::string Line;
		std::string Sub;
		if (!W.Started)
		{
			Line = "100% up to $100";
			Sub = "your first deposit starts it";
		}
		else if (W.Closed)
		{
			Line = Money(W.Released) + " released";
			Sub = "the 60 days are up";
		}
		else if (W.Done)
		{
			Line = Money(W.Released) + " released";
			Sub = "all of it's yours";
		}
		else
		{
			Line = Money(W.Released) + " of " + Money(W.Released + W.Pending);
			Sub = W.NextIn > 0 ? "next $5 in " + Money(W.NextIn) + " of rake" : "$5 for every $20 of rake";
		}
		const double Needle = W.Started ? W.Needle : 0.0;
		const bool Hot = St.Hover;
		const std::string Look = "ignite:dockband|" + Line + "|" + Sub + (Hot ? "|hot|" : "|") + Fixed(Needle, 6);
		C.Cached((static_cast<uint64_t>(Fnv1a(Look)) << 16) ^ Tone, 0.0f, 0.0f, [&K, &Line, &Sub, &R, Needle, Hot](Canvas& At) {
			At.FillRoundRect(R, 20.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, IgniteHx(Hot ? K.Panel : K.PanelTop), IgniteHx(K.PanelBot)));
			At.StrokeRoundRect(R, 20.0f, IgniteHx(Hot ? K.Coral : K.Rim), 1.0f);
			IgniteFuelGauge(At, 70.0f, 934.0f, 30.0f, Needle, K);
			At.Text("WELCOME BONUS", 116.0f, 906.0f, IgniteBi(13.0f, Hex(K.Cream)));
			At.Text(Line, 116.0f, 926.0f, IgniteTx(13.0f, 800, Hex(IgLime), Align::Left, Baseline::Alphabetic, 236.0f));
			At.Text(Sub, 116.0f, 941.0f, IgniteTx(11.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, 236.0f));
		});
		if (St.Clicked)
		{
			IgniteOf(&X.RL).Rewards = 3;
			Access::OpenPage(X.RL, RiverLine::Page::Promotions, X.Now);
		}
	}
	// The dock's pill and its four flat items.
	struct DockItem
	{
		float X;
		const char* Label;
		int Icon;
		RiverLine::Page P;
		int Nav;
	};
	const DockItem Items[4] = {{400.0f, "PLAY", 0, RiverLine::Page::Lobby, 0}, {556.0f, "REWARDS", 1, RiverLine::Page::Promotions, 1}, {904.0f, "BOARDS", 2, RiverLine::Page::Leaderboards, 3},
		{1060.0f, "WALLET", 3, RiverLine::Page::Cashier, 4}};
	Ui::ClickState Sts[4];
	uint64_t States = 0;
	for (int I = 0; I < 4; ++I)
	{
		const DockItem& It = Items[I];
		Sts[I] = UI.Clickable("nav" + std::to_string(It.Nav), {It.X, 888.0f, 140.0f, 56.0f}, !X.Preview);
		const bool Active = Pg == It.P && !(It.P == RiverLine::Page::Lobby && FlipChip);
		States |= (Active ? 1ull : 0ull) << (2 * I);
		States |= (Sts[I].Hover ? 2ull : 0ull) << (2 * I);
	}
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:dock")) << 24) ^ (States << 16) ^ Tone, 0.0f, 0.0f, [&K, &Items, States](Canvas& At) {
		At.FillRoundRect({IgniteDockX, IgniteDockY + 5.0f, IgniteDockW, IgniteDockH}, 32.0f, Rgba(0, 0, 0, 0.35f));
		At.FillRoundRect({IgniteDockX, IgniteDockY, IgniteDockW, IgniteDockH}, 32.0f, IgniteHx(IgNight, 0.96f));
		At.StrokeRoundRect({IgniteDockX, IgniteDockY, IgniteDockW, IgniteDockH}, 32.0f, IgniteHx(K.RimHi), 1.5f);
		At.FillRect({IgniteDockX + 12.0f, IgniteDockY + 2.0f, IgniteDockW - 24.0f, 2.0f}, Rgba(255, 255, 255, 0.06f));
		for (int I = 0; I < 4; ++I)
		{
			const DockItem& It = Items[I];
			const bool Active = ((States >> (2 * I)) & 1ull) != 0;
			const bool Hover = ((States >> (2 * I)) & 2ull) != 0;
			const float Y = 888.0f - (Active ? 10.0f : 0.0f);
			const Rect R{It.X, Y, 140.0f, 56.0f};
			if (Active)
			{
				At.GlowRoundRect({R.X + 6.0f, R.Y + 48.0f, R.W - 12.0f, 20.0f}, 10.0f, IgniteHx(K.Coral, 0.55f), 12.0f);
				At.FillRoundRect(R, 28.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, IgniteHx(K.CoralLight), IgniteHx(K.CoralDark)));
				At.FillRoundRect({R.X + 10.0f, R.Y + 3.0f, R.W - 20.0f, 3.0f}, 1.5f, Rgba(255, 255, 255, 0.35f));
			}
			else if (Hover)
			{
				At.FillRoundRect(R, 28.0f, IgniteHx(K.Panel));
			}
			// The lit item's label in the chips' dark ink (cream on the coral-to-light gradient reads under 3:1, at night 1.3:1).
			const Color Ink = Active ? IgniteHx(0x2a0d14) : Hover ? Hex(K.Cream) : pal::Muted;
			IgniteDockIcon(At, It.Icon, It.X + 34.0f, Y + 28.0f, Ink, IgniteHx(IgNight));
			At.Text(It.Label, It.X + 54.0f, Y + 33.0f, IgniteTx(14.5f, Active ? 900 : 800, Ink));
		}
	});
	for (int I = 0; I < 4; ++I)
	{
		const DockItem& It = Items[I];
		const Ui::ClickState& St = Sts[I];
		if (St.Clicked)
		{
			if (It.P == RiverLine::Page::Lobby && FlipChip)
			{
				IgniteOf(&X.RL).Chip = 0;
				IgniteOf(&X.RL).ChipAt = X.Now;
			}
			if (It.P == RiverLine::Page::Promotions)
			{
				IgniteOf(&X.RL).Rewards = 3;
			}
			Access::OpenPage(X.RL, It.P, X.Now);
		}
	}
	// The raised centre button. G1 (and if S-J fails): FLIP, opening PLAY on Flip & Run (nav2 keeps its id).
	{
		// The button breathes on the beat in 12 steps (an eighth of a unit each), so its rings replay from the cache by the step,
		// the pointer and whether FLIP is on (G1_AUDIT M3); the coin spins live over them.
		const int Step = std::min(11, static_cast<int>(IgnitePulse(X) * 12.0f));
		const float Breath = 1.0f + 0.025f * (static_cast<float>(Step) + 0.5f) / 12.0f;
		const bool Active = Pg == RiverLine::Page::Lobby && FlipChip;
		const Ui::ClickState St = UI.Clickable("nav2", {744.0f, 836.0f, 112.0f, 112.0f}, !X.Preview);
		const float R0 = IgniteRaisedR * Breath * (St.Down ? 0.94f : 1.0f);
		const bool Hot = St.Hover;
		const uint64_t Look = (static_cast<uint64_t>(Fnv1a("ignite:raised")) << 16) ^ (static_cast<uint64_t>(Step) << 8) ^ (St.Down ? 0x80u : 0u) ^ (Hot ? 0x40u : 0u) ^ (Active ? 0x20u : 0u) ^ Tone;
		C.Cached(Look, 0.0f, 0.0f, [&K, R0, Hot, Active](Canvas& At) {
			At.GlowRoundRect({800.0f - 68.0f, 892.0f - 68.0f, 136.0f, 136.0f}, 68.0f, IgniteHx(K.Coral, Active ? 0.42f : 0.3f), 30.0f);
			At.FillCircle(IgniteRaisedCx, IgniteRaisedCy + 5.0f, 60.0f, Rgba(0, 0, 0, 0.4f));
			At.FillCircle(IgniteRaisedCx, IgniteRaisedCy, R0 + 7.0f, IgniteHx(IgPink, Hot ? 0.85f : 0.5f));
			At.FillCircle(IgniteRaisedCx, IgniteRaisedCy, R0 + 5.0f, IgniteHx(IgNight));
			At.FillCircle(IgniteRaisedCx, IgniteRaisedCy, R0 - 1.0f, IgniteHx(K.Cream, 0.45f));
			At.FillCircle(IgniteRaisedCx, IgniteRaisedCy, R0 - 3.0f, Paint::Linear({783.0f, 840.0f}, {817.0f, 944.0f}, IgniteHx(K.CoralLight), IgniteHx(K.CoralDark)));
			At.Text("FLIP", 800.0f, 916.0f, IgniteBi(24.0f, Hex(K.Cream), Align::Center));
		});
		// The coin flipping on its edge (a Flip & Run's all-in at the start), on the beat.
		const float Spin = static_cast<float>(std::fabs(std::cos(motion::Beat(X.Tm, K.Bpm * 0.5) * 3.14159265358979)));
		C.FillEllipse(800.0f, 872.0f, 15.0f * std::max(0.18f, Spin), 15.0f, Hex(IgSun));
		C.StrokeEllipse(800.0f, 872.0f, 10.5f * std::max(0.18f, Spin), 10.5f, IgniteHx(0x7a4a00, 0.7f), 2.0f);
		if (St.Clicked)
		{
			IgniteUi& U = IgniteOf(&X.RL);
			U.Chip = 2;
			U.ChipAt = X.Now;
			U.FloorGoal = 0.0f;
			X.S.ShowLobby();
			Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
		}
	}
	// G1, right: where the wheel and the crackers will sit, the Sunday Blaze's countdown; it opens the Blaze's sheet.
	{
		const Rect R{1236.0f, 884.0f, 340.0f, 64.0f};
		net::EventInstance Bz;
		const bool Has = IgniteBlazeNow(X, Bz);
		const Ui::ClickState St = UI.Clickable("dockblaze", R, Has && !X.Preview);
		const bool Hot = St.Hover;
		C.Cached((static_cast<uint64_t>(Fnv1a(Hot ? "ignite:dockblaze|hot" : "ignite:dockblaze")) << 16) ^ Tone, 0.0f, 0.0f, [&K, &R, Hot](Canvas& At) {
			At.FillRoundRect(R, 20.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
			At.StrokeRoundRect(R, 20.0f, IgniteHx(Hot ? K.Coral : K.Rim), 1.0f);
		});
		C.GlowRoundRect({1252.0f, 896.0f, 40.0f, 40.0f}, 20.0f, IgniteHx(IgTang, 0.25f + 0.15f * IgnitePulse(X)), 10.0f);
		C.Cached(static_cast<uint64_t>(Fnv1a("ignite:dockblaze-flame")) << 16, 0.0f, 0.0f, [](Canvas& At) {
			IgniteFlame(At, 1272.0f, 935.0f, 22.0f, Hex(IgTang), Hex(IgSun), Hex(IgCream));
			At.Text("SUNDAY BLAZE", 1304.0f, 911.0f, IgniteBi(14.0f, Hex(IgSun)));
		});
		if (Has)
		{
			const net::EventTemplate& T = Net.TemplateOf(Bz);
			const net::LiveState L = Net.Live(Bz, X.World);
			const bool Running = L.St == net::Status::LateReg || L.St == net::Status::Running || L.St == net::Status::FinalTable;
			const std::string When = Running ? std::string("ON NOW") : IgniteSignTime(Bz.Start - X.World);
			C.Text(When, 1560.0f, 911.0f, IgniteBi(14.0f, Hex(K.Cream), Align::Right));
			C.Text(net::BuyIn(T.BuyInCents) + " \xC2\xB7 GTD " + rlnet_detail::NetMoney(T.GtdCents) + " \xC2\xB7 " + IgniteWhen(Bz.Start, X.World), 1304.0f, 931.0f,
				IgniteTx(11.0f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 256.0f));
			if (St.Clicked)
			{
				IgniteOpenSheet(X.RL, Bz.Id, X.Now);
				Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
			}
		}
	}
}

// ------------------------------------------------------------------ PLAY: the Boardwalk (SPEC §2.4 A)

/** The series running today on Ignite (Wildfire), or null. */
const net::SeriesInfo* IgniteWildfire(const net::Network& Net, double World)
{
	const net::SeriesInfo* Sr = Net.CurrentSeries(World);
	const int Today = net::DayOf(World);
	return Sr && Today >= Sr->FirstDay && Today <= Sr->LastDay ? Sr : nullptr;
}

/** A sign's hotspot: a glow when the pointer's over it; true on a click. */
bool IgniteHotspot(IgniteCtx& X, const std::string& Id, const Rect& R, float Radius)
{
	const Ui::ClickState St = X.UI.Clickable(Id, R, !X.Preview);
	if (St.Hover)
	{
		X.C.GlowRoundRect(R, Radius, IgniteHx(X.K.Coral, 0.16f), 14.0f);
	}
	return St.Clicked;
}

void IgniteBoardwalk(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	IgniteBand B;
	B.K = K;
	B.Now = X.Tm;
	B.World = X.World;
	{
		int Year = 0;
		int Month = 0;
		int Date = 0;
		world::CivilDate(net::DayOf(X.World), Year, Month, Date);
		B.Snow = Month == 12 || Month <= 2;
	}
	net::EventInstance Bz;
	const bool HasBlaze = IgniteBlazeNow(X, Bz);
	const net::LiveState BzL = HasBlaze ? Net.Live(Bz, X.World) : net::LiveState();
	const bool BlazeOn = HasBlaze && (BzL.St == net::Status::LateReg || BzL.St == net::Status::Running || BzL.St == net::Status::FinalTable);
	B.Fireworks = BlazeOn && X.World - Bz.Start < 120.0;
	const net::SeriesInfo* Fire = IgniteWildfire(Net, X.World);
	B.Wildfire = Fire != nullptr;
	B.SkinKey = Skin::Current().Key();
	// The static skyline, cached in the Boardwalk pool per mood and skin (≤ 12k vertices, SPEC §10).
	{
		uint64_t Key = 0xcbf29ce484222325ull;
		const uint32_t Parts[3] = {static_cast<uint32_t>(K.Mood), K.Night ? 1u : 0u, 0x1640a7u};
		for (uint32_t P : Parts)
		{
			Key = (Key ^ P) * 0x100000001b3ull;
		}
		Key = (Key ^ B.SkinKey) * 0x100000001b3ull;
		C.Cached(Key, 0.0f, 0.0f, [&B](Canvas& At) { IgniteBoardwalkStatic(At, B); }, Canvas::CachePool::Boardwalk);
	}
	IgniteBoardwalkLive(C, B);
	// The hour's word in the sky (neon), and its beat (still for the hour: it replays with the band, G1_AUDIT M3).
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:hour-word")) << 8) ^ static_cast<uint64_t>(K.Mood), 0.0f, 0.0f, [&K](Canvas& At) {
		IgniteCaps(At, K.Label, 1576.0f, 92.0f, 13.0f, 900, Hex(K.Glow), 3.0f, Align::Right);
		At.Text(std::to_string(static_cast<int>(K.Bpm)) + " BPM", 1576.0f, 106.0f, IgniteTx(8.5f, 700, IgniteHx(K.Cream, 0.5f), Align::Right));
	}, Canvas::CachePool::Boardwalk);

	// The Lighthouse: the Sunday Blaze's countdown (or, while Wildfire runs, the series' sign, opening SERIES).
	{
		const Rect Sign{16.0f, 268.0f, 152.0f, 28.0f};
		if (IgniteHotspot(X, "iglight", {28.0f, 88.0f, 128.0f, 208.0f}, 18.0f) || IgniteHotspot(X, "iglightsign", Sign, 14.0f))
		{
			if (Fire)
			{
				Access::OpenPage(X.RL, RiverLine::Page::Series, X.Now);
			}
			else if (HasBlaze)
			{
				IgniteOpenSheet(X.RL, Bz.Id, X.Now);
			}
		}
		const bool Series = Fire != nullptr;
		C.Cached((static_cast<uint64_t>(Fnv1a(Series ? "ignite:lighthouse-sign|series" : "ignite:lighthouse-sign")) << 32) ^ static_cast<uint64_t>(K.Coral), 0.0f, 0.0f, [&K, &Sign, Series](Canvas& At) {
			At.FillRoundRect(Sign, 14.0f, IgniteHx(0x1a0f2a));
			At.StrokeRoundRect(Sign, 14.0f, Series ? Hex(IgTang) : Hex(IgSun), 1.5f);
			IgniteFlame(At, 31.0f, 289.0f, 11.0f, Hex(K.Coral), Hex(IgTang), Hex(IgSun));
		});
		std::string Line;
		if (Fire)
		{
			Line = Fire->Short + " \xC2\xB7 DAY " + std::to_string(net::DayOf(X.World) - Fire->FirstDay + 1) + " OF " + std::to_string(Fire->LastDay - Fire->FirstDay + 1);
		}
		else if (HasBlaze)
		{
			Line = "SUNDAY BLAZE \xC2\xB7 " + (BlazeOn ? std::string("LIVE") : IgniteSignTime(Bz.Start - X.World));
		}
		else
		{
			Line = "SUNDAY BLAZE";
		}
		C.Text(Line, 42.0f, 286.5f, IgniteTx(10.0f, 900, Fire ? Hex(IgTang) : Hex(IgSun), Align::Left, Baseline::Alphabetic, 120.0f));
	}

	// The Bandstand: TONIGHT (the slide-1 logic: the night's featured event), else what's up next.
	{
		const std::vector<Access::FeatureSlide>& Slides = Access::FeatureSlides(X.RL);
		net::EventInstance E;
		bool Has = false;
		bool Tonight = false;
		if (!Slides.empty() && Slides.front().HasEvent && Slides.front().Art == 0)
		{
			E = Slides.front().E;
			Has = Tonight = true;
		}
		else
		{
			const std::vector<net::EventInstance> Next = IgniteFeaturedNow(X, 1);
			if (!Next.empty())
			{
				E = Next.front();
				Has = true;
			}
		}
		if (IgniteHotspot(X, "igband", {184.0f, 104.0f, 444.0f, 196.0f}, 24.0f) && Has)
		{
			IgniteOpenSheet(X.RL, E.Id, X.Now);
		}
		C.Text(Tonight ? "TONIGHT \xC2\xB7 BANDSTAND" : "UP NEXT \xC2\xB7 BANDSTAND", 230.0f, 160.0f, IgniteTx(11.5f, 800, Hex(IgLime)));
		if (Has)
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			const net::LiveState L = Net.Live(E, X.World);
			C.Text(T.Name, 230.0f, 196.0f, IgniteBi(32.0f, Hex(IgSun), Align::Left, Baseline::Alphabetic, 226.0f));
			std::string Facts = IgniteWhen(E.Start, X.World) + " \xC2\xB7 " + net::BuyIn(T.BuyInCents);
			Facts += T.GtdCents > 0 ? " \xC2\xB7 GTD " + rlnet_detail::NetMoney(T.GtdCents) : std::string();
			Facts += " \xC2\xB7 " + Grouped(L.Entries) + " in";
			C.Text(Facts, 230.0f, 218.0f, IgniteTx(14.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 354.0f));
			std::string Clock;
			switch (L.St)
			{
			case net::Status::Announced: Clock = "IN " + rlnet_detail::NetHms(E.Start - X.World); break;
			case net::Status::Registering: Clock = "STARTS " + rlnet_detail::NetHms(L.StartsIn); break;
			case net::Status::LateReg: Clock = "LATE " + rlnet_detail::NetHms(L.LateRegLeft); break;
			default: Clock = "ON NOW"; break;
			}
			const float Cw = std::max(110.0f, C.Measure(Clock, 12.5f, 900) + 26.0f);
			C.FillRoundRect({584.0f - Cw, 150.0f, Cw, 30.0f}, 15.0f, Hex(K.Coral));
			C.Text(Clock, 584.0f - Cw / 2.0f, 170.0f, IgniteTx(12.5f, 900, IgniteHx(0x2a0d14), Align::Center));
			// WHO'S IN: the regulars the world has registered (the Headliners join them at G2).
			C.FillRect({230.0f, 232.0f, 354.0f, 1.0f}, IgniteHx(K.Rim));
			C.Text("WHO'S IN", 230.0f, 250.0f, IgniteTx(10.0f, 900, Hex(igniteart::IgPink)));
			std::vector<int> Who = X.S.Living().Registered(E.Id);
			const world::World& W = X.S.Living();
			std::stable_sort(Who.begin(), Who.end(), [&W](int A, int Bb) { return W.Get(A)->RepOf(world::Rep::Overall) > W.Get(Bb)->RepOf(world::Rep::Overall); });
			if (Who.empty())
			{
				C.Text(L.St == net::Status::Announced ? IgniteSeatsOpen : IgniteNoRegulars, 230.0f, 272.0f, IgniteTx(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, 354.0f));
			}
			float Wx = 246.0f;
			for (size_t I = 0; I < Who.size() && I < 3; ++I)
			{
				const world::Npc* N = W.Get(Who[I]);
				const AvatarSpec Pic = AvatarFor(N->Name);
				DrawAvatar(C, Wx, 268.0f, 12.0f, Pic);
				C.StrokeArc(Wx, 268.0f, 12.5f, 0.0f, 2.0f * Pi, Hex(igniteart::IgSun), 1.5f);
				const float Nw = C.Text(N->Name, Wx + 18.0f, 272.0f, IgniteTx(12.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 96.0f));
				Wx += 18.0f + Nw + 22.0f;
			}
			if (Who.size() > 3)
			{
				C.Text("+" + std::to_string(Who.size() - 3), Wx, 272.0f, IgniteTx(12.0f, 800, pal::Muted));
			}
		}
		else
		{
			C.Text("The night's lineup", 230.0f, 200.0f, IgniteBi(26.0f, Hex(IgSun)));
		}
	}

	// The Spark Wheel. G1: three gondolas carry tonight's next featured starts; a click opens the nearest's sheet.
	{
		const std::vector<net::EventInstance> Starts = IgniteFeaturedNow(X, 3);
		std::vector<std::string> Labels;
		bool AllTonight = true;
		for (const net::EventInstance& E : Starts)
		{
			std::string Nm = Net.TemplateOf(E).Name;
			if (Nm.size() > 14)
			{
				// Cut on a character's first byte, never inside one (a name may carry an accent).
				size_t Cut = 13;
				while (Cut > 0 && (static_cast<unsigned char>(Nm[Cut]) & 0xC0) == 0x80)
				{
					--Cut;
				}
				Nm = Nm.substr(0, Cut) + "\xE2\x80\xA6";
			}
			// A start that isn't today's carries its weekday ("Thu 8 PM"), and the sign says "tonight" only of tonight's.
			AllTonight = AllTonight && IgniteTonight(E.Start, X.World);
			Labels.push_back(IgniteWhen(E.Start, X.World) + " \xC2\xB7 " + Nm);
		}
		const float Angle = static_cast<float>(std::fmod(X.Tm / 90.0, 1.0) * 2.0 * 3.14159265358979) + 0.1f;
		IgniteFerrisWheel(C, IgniteWheelCx, IgniteWheelCy, IgniteWheelR, Angle, Labels, K, X.Tm);
		if (IgniteHotspot(X, "igwheel", {676.0f, 100.0f, 228.0f, 228.0f}, 114.0f) && !Starts.empty())
		{
			IgniteOpenSheet(X.RL, Starts.front().Id, X.Now);
		}
		const Rect Sign = IgniteWheelSign;
		const std::string Says = Starts.empty() ? std::string("THE SPARK WHEEL") : (AllTonight ? "TONIGHT'S STARTS \xC2\xB7 " : "NEXT STARTS \xC2\xB7 ") + std::to_string(Starts.size());
		C.Cached(static_cast<uint64_t>(Fnv1a("ignite:wheel-sign|" + Says)) << 8, 0.0f, 0.0f, [&Sign, &Says](Canvas& At) {
			At.FillRoundRect(Sign, 12.0f, IgniteHx(0x1a0f2a));
			At.StrokeRoundRect(Sign, 12.0f, Hex(IgLime), 1.5f);
			At.Text(Says, 790.0f, 308.0f, IgniteTx(10.5f, 900, Hex(IgLime), Align::Center));
		});
	}

	// The Fuse Booth. G1: the next Flip & Run is on its sign, and a click opens that Flip's sheet.
	{
		const IgniteUi::SchedulePicks& Picks = IgnitePicksOf(X);
		const net::EventInstance& F = Picks.Flip;
		const bool Has = Picks.HasFlip;
		if (IgniteHotspot(X, "igbooth", {956.0f, 102.0f, 252.0f, 196.0f}, 18.0f) && Has)
		{
			IgniteOpenSheet(X.RL, F.Id, X.Now);
		}
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:booth-sign")) << 32) ^ static_cast<uint64_t>(K.Cream), 0.0f, 0.0f, [&K](Canvas& At) {
			At.FillRoundRect({1010.0f, 180.0f, 144.0f, 22.0f}, 4.0f, Hex(IgSun));
			At.Text("FLIP & RUN", 1082.0f, 196.0f, IgniteTx(12.0f, 900, IgniteHx(0x2b1240), Align::Center));
			At.Text("NEXT FLIP", 1082.0f, 232.0f, IgniteBi(21.0f, Hex(K.Cream), Align::Center));
		});
		if (Has)
		{
			const net::LiveState L = Net.Live(F, X.World);
			const net::EventTemplate& T = Net.TemplateOf(F);
			C.Text(IgniteUpper(rlnet_detail::LiveTime(F.Start)) + " \xC2\xB7 " + net::BuyIn(T.BuyInCents) + " \xC2\xB7 " + Grouped(L.Entries) + " IN", 1082.0f, 247.0f, IgniteTx(10.0f, 800, Hex(IgSun), Align::Center));
			const double Wait = L.St == net::Status::LateReg ? 0.0 : std::max(0.0, F.Start - X.World);
			// The rope by how far it has burnt (a point a few minutes), from the cache; its spark live (G1_AUDIT M3).
			const double Burn = 1.0 - Clamp01(Wait / 60.0);
			const int Head = IgniteFuseHead(1004.0f, 1142.0f, Burn);
			C.Cached((static_cast<uint64_t>(Fnv1a("ignite:booth-fuse")) << 8) ^ static_cast<uint64_t>(Head), 0.0f, 0.0f, [Burn](Canvas& At) {
				IgniteFuseRope(At, 1004.0f, 1142.0f, 262.0f, 4.0f, Burn, Hex(0xc9a46a), IgniteHx(0x5a4060), 0.0, false);
			});
			IgniteFuseSpark(C, 1004.0f, 1142.0f, 262.0f, 4.0f, Burn, X.Tm);
			C.Text(L.St == net::Status::LateReg ? "LATE REG " + rlnet_detail::NetHms(L.LateRegLeft) : "IN " + rlnet_detail::NetHms(Wait), 1004.0f, 291.0f, IgniteTx(10.0f, 900, pal::Muted));
		}
	}

	// The Prize Counter. G1: its strings hang the welcome bonus (released, pending, the next $5); it opens that page.
	{
		if (IgniteHotspot(X, "igprize", {1244.0f, 104.0f, 320.0f, 194.0f}, 12.0f))
		{
			IgniteOf(&X.RL).Rewards = 3;
			Access::OpenPage(X.RL, RiverLine::Page::Promotions, X.Now);
		}
		const IgniteWelcome W = IgniteWelcomeOf(X.S, X.World);
		struct Hanging
		{
			std::string Top, Bottom;
			uint32_t Col;
		};
		std::vector<Hanging> Strings;
		if (!W.Started)
		{
			Strings = {{"100%", "MATCHED", igniteart::IgLime}, {"$100", "UP TO", igniteart::IgSun}, {"$5", "PER $20", K.Coral}};
		}
		else
		{
			Strings.push_back({rlnet_detail::NetMoney(W.Released), "RELEASED", igniteart::IgLime});
			Strings.push_back({rlnet_detail::NetMoney(W.Pending), W.Closed ? "EXPIRED" : "PENDING", igniteart::IgSun});
			if (!W.Closed && !W.Done)
			{
				Strings.push_back({"$5", W.NextIn > 0 ? "NEXT" : "PER $20", K.Coral});
			}
		}
		const float Xs[3] = {1262.0f, 1338.0f, 1418.0f};
		const float Ys[3] = {156.0f, 168.0f, 158.0f};
		const size_t Hung = std::min<size_t>(Strings.size(), 3);
		// The counter's head and its strings are still; each ticket sways as one piece (upright, so it replays from the
		// cache wherever the sway puts it: G1_AUDIT M3). The strings sit clear of every ticket but their own.
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:prize-head")) << 8) ^ static_cast<uint64_t>(Hung), 0.0f, 0.0f, [&Xs, &Ys, Hung](Canvas& At) {
			At.FillRoundRect({1316.0f, 112.0f, 178.0f, 18.0f}, 9.0f, IgniteHx(IgNight, 0.85f));
			At.Text("PRIZE COUNTER", 1405.0f, 125.0f, IgniteTx(11.5f, 900, Hex(IgSun), Align::Center));
			for (size_t I = 0; I < Hung; ++I)
			{
				At.FillRect({Xs[I] + 34.0f, 142.0f, 1.0f, Ys[I] - 142.0f}, IgniteHx(0xc9b8e6, 0.6f));
			}
		});
		for (size_t I = 0; I < Hung; ++I)
		{
			// The strings sway (the geometry, ±2°); the labels stay upright.
			const float Sway = static_cast<float>(std::sin(X.Tm * 1.3 + static_cast<double>(I) * 1.7) * 2.0);
			const float Hx = Xs[I] + Sway;
			const Hanging& Hg = Strings[I];
			C.Cached((static_cast<uint64_t>(Fnv1a("ignite:ticket|" + Hg.Top + "|" + Hg.Bottom)) << 24) ^ static_cast<uint64_t>(Hg.Col), Hx, Ys[I], [&Hg](Canvas& At) {
				IgniteTicket(At, 0.0f, 0.0f, 70.0f, 38.0f, Hg.Top, Hg.Bottom, Hex(Hg.Col), IgniteHx(0x1a0f2a));
			});
		}
		const std::string Sub = !W.Started ? std::string("Your first deposit starts it.") : W.Closed ? std::string("The 60 days are up.")
			: W.Done ? std::string(IgniteAllYours) : W.NextIn > 0 ? "next $5 in " + Money(W.NextIn) + " of rake" : std::string("$5 for every $20 of rake");
		const std::string Released = W.Started ? Money(W.Released) : std::string("$0.00");
		const double Needle = W.Started ? W.Needle : 0.0;
		const uint64_t Tone = (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u) ^ (Skin::Current().Key() << 4);
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:prize-foot|" + Sub + "|" + Released + "|" + Fixed(Needle, 6))) << 16) ^ Tone, 0.0f, 0.0f, [&K, &Sub, &Released, Needle](Canvas& At) {
			At.Text("Welcome bonus", 1256.0f, 268.0f, IgniteTx(12.0f, 700, Hex(K.Cream)));
			At.Text(Sub, 1256.0f, 285.0f, IgniteTx(10.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 124.0f));
			At.FillRoundRect({1384.0f, 256.0f, 168.0f, 32.0f}, 6.0f, IgniteHx(0x0f1a08));
			At.StrokeRoundRect({1384.0f, 256.0f, 168.0f, 32.0f}, 6.0f, IgniteHx(igniteart::IgLime, 0.6f), 1.2f);
			IgniteFuelGauge(At, 1404.0f, 282.0f, 14.0f, Needle, K);
			At.Text(Released, 1424.0f, 279.0f, IgniteBi(17.0f, Hex(igniteart::IgLime)));
			At.Text("RELEASED", 1546.0f, 278.0f, IgniteTx(8.5f, 800, IgniteHx(igniteart::IgLime, 0.75f), Align::Right));
		});
	}
}

// ------------------------------------------------------------------ PLAY: the Today strip, the chips, the shelves

/** The Today strip. G1: tonight's three featured events (the Flame, the Dares and the near-goal line are G2); under the
 * clause, the contract's line instead. */
void IgniteToday(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const std::string Clause = IgniteClauseBanner(X.S);
	if (!Clause.empty())
	{
		const Rect R{24.0f, 364.0f, 1552.0f, 64.0f};
		C.FillRoundRect(R, 18.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, IgniteHx(IgRed, 0.16f), IgniteHx(K.PanelBot)));
		C.StrokeRoundRect(R, 18.0f, IgniteHx(IgRed, 0.6f), 1.5f);
		IgniteSticker(C, 62.0f, 396.0f, 20.0f, "!", Hex(IgRed), Hex(IgDarkInk), 20.0f, 12, 0.72f);
		C.Text(Clause, 98.0f, 403.0f, IgniteTx(17.0f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 1440.0f));
		return;
	}
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	const std::vector<net::EventInstance> Feat = IgniteFeaturedNow(X, 3);
	const float W = (1552.0f - 2.0f * 16.0f) / 3.0f;
	for (size_t I = 0; I < 3; ++I)
	{
		const Rect R{24.0f + static_cast<float>(I) * (W + 16.0f), 364.0f, W, 64.0f};
		if (I >= Feat.size())
		{
			C.FillRoundRect(R, 18.0f, IgniteHx(K.PanelBot, 0.6f));
			C.StrokeRoundRect(R, 18.0f, IgniteHx(K.Rim, 0.6f), 1.0f);
			continue;
		}
		const net::EventInstance& E = Feat[I];
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, X.World);
		const Ui::ClickState St = X.UI.Clickable("today" + std::to_string(I), R, !X.Preview);
		const bool Late = L.St == net::Status::LateReg;
		const bool Hot = St.Hover;
		const std::string Kicker = Late ? "LATE REG \xC2\xB7 " + rlnet_detail::NetHms(L.LateRegLeft) : IgniteUpper(IgniteWhen(E.Start, X.World)) + (T.Featured ? " \xC2\xB7 FEATURED" : " \xC2\xB7 GUARANTEED");
		// The card replays from the cache (its event's mark is a few hundred vertices a frame: G1_AUDIT M3); what ticks
		// (late registration's countdown, a field still filling) draws live, in the order the card always drew it.
		const std::string Look = "ignite:today:" + T.Id + "|" + (Late ? std::string("late") : Kicker) + (Hot ? "|hot" : "") + "|" + std::to_string(I);
		C.Cached((static_cast<uint64_t>(Fnv1a(Look)) << 8) ^ (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u), 0.0f, 0.0f, [&T, &K, &Kicker, Late, Hot, &R](Canvas& At) {
			At.FillRoundRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, IgniteHx(Hot ? K.Panel : K.PanelTop), IgniteHx(K.PanelBot)));
			At.StrokeRoundRect(R, 18.0f, Late ? Hex(IgLime) : Hot ? Hex(K.Coral) : IgniteHx(K.RimHi), Late || Hot ? 1.5f : 1.0f);
			IgniteEventMark(At, T, R.X + 34.0f, R.Y + 32.0f, 19.0f, K);
			if (!Late)
			{
				At.Text(Kicker, R.X + 62.0f, R.Y + 24.0f, IgniteTx(10.0f, 900, Hex(K.Sky)));
			}
			At.Text(T.Name, R.X + 62.0f, R.Y + 46.0f, IgniteTx(17.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, R.W - 200.0f));
			At.Text(net::BuyIn(T.BuyInCents), R.X + R.W - 22.0f, R.Y + 34.0f, IgniteBi(20.0f, Hex(K.Cream), Align::Right));
			if (T.GtdCents > 0)
			{
				At.Text("GTD " + rlnet_detail::NetMoney(T.GtdCents), R.X + R.W - 22.0f, R.Y + 52.0f, IgniteTx(11.5f, 800, Hex(IgSun), Align::Right));
			}
		});
		if (Late)
		{
			C.Text(Kicker, R.X + 62.0f, R.Y + 24.0f, IgniteTx(10.0f, 900, Hex(IgLime)));
		}
		if (T.GtdCents <= 0)
		{
			C.Text(Grouped(L.Entries) + " in", R.X + R.W - 22.0f, R.Y + 52.0f, IgniteTx(11.5f, 800, Hex(IgSun), Align::Right));
		}
		if (St.Clicked)
		{
			IgniteOpenSheet(X.RL, E.Id, X.Now);
		}
	}
}

/** The filter chips (ids chip<Name>, as RiverLine's); FUSE stays hidden until FUSE lights (G2). */
const char* const IgniteChipNames[10] = {"All", "Starting soon", "Flip & Run", "Bounty", "Mystery", "$1-3", "$5-22", "Guaranteed", "FUSE", "My tickets"};

bool IgniteChipKeeps(int Chip, const net::EventTemplate& T, const net::LiveState& L, const Session& S, const net::Network& Net)
{
	switch (Chip)
	{
	case 1: return L.St == net::Status::LateReg || (L.St == net::Status::Registering && L.StartsIn <= 60.0);
	case 2: return T.Fmt == net::Format::Flip;
	case 3: return T.Fmt == net::Format::Bounty;
	case 4: return T.Fmt == net::Format::Mystery;
	case 5: return T.BuyInCents > 0 && T.BuyInCents <= 330;
	case 6: return T.BuyInCents >= 550 && T.BuyInCents <= 2200;
	case 7: return T.GtdCents > 0;
	case 9: return S.Life.TicketsFor(Net.TicketOf(T.Id)) > 0;
	default: return true;
	}
}

void IgniteChips(IgniteCtx& X, int Count)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	// The pointer first (each chip's hit area), then the row replays from the cache by the chip that's on, the one under the
	// pointer, the count and the hour's tones (G1_AUDIT M3).
	Rect Rs[10];
	int Hot = -1;
	const int Was = X.U.Chip;
	float Cx = 24.0f;
	for (int I = 0; I < 10; ++I)
	{
		if (I == 8)
		{
			continue; // FUSE: G2 (it jumps to the Fuse Booth once FUSE lights)
		}
		const float W = C.Measure(IgniteChipNames[I], 13.5f, 700) + 30.0f;
		Rs[I] = {Cx, 440.0f, W, 32.0f};
		const Ui::ClickState St = X.UI.Clickable(std::string("chip") + IgniteChipNames[I], Rs[I], !X.Preview);
		Hot = St.Hover ? I : Hot;
		if (St.Clicked && X.U.Chip != I)
		{
			X.U.Chip = I;
			X.U.ChipAt = X.Now;
			X.U.FloorGoal = 0.0f;
			X.U.ShelfGoal.fill(0.0f);
		}
		Cx += W + 8.0f;
	}
	const std::string Floor = Grouped(Count) + (Count == 1 ? " event on the floor" : " events on the floor");
	const uint64_t Key = (static_cast<uint64_t>(Fnv1a("ignite:chips|" + Floor)) << 16) ^ (static_cast<uint64_t>(Was) << 8) ^ (static_cast<uint64_t>(Hot + 1) << 4) ^
		(static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u) ^ (Skin::Current().Key() << 20);
	C.Cached(Key, 0.0f, 0.0f, [&K, &Rs, &Floor, Was, Hot](Canvas& At) {
		for (int I = 0; I < 10; ++I)
		{
			if (I == 8)
			{
				continue;
			}
			const Rect& R = Rs[I];
			const bool On = Was == I;
			if (On)
			{
				At.GlowRoundRect(R, 16.0f, IgniteHx(K.Coral, 0.3f), 10.0f);
				igniteart::IgnitePill(At, R, Hex(K.Coral), Color(), 0.0f);
			}
			else
			{
				igniteart::IgnitePill(At, R, IgniteHx(Hot == I ? K.Panel : K.Well), IgniteHx(K.RimHi), Hot == I ? 1.2f : 0.0f);
			}
			At.Text(IgniteChipNames[I], R.X + R.W / 2.0f, R.Y + 21.0f, igniteart::IgniteTx(13.5f, On ? 800 : 600, On ? IgniteHx(0x2a0d14) : Hex(K.Cream), Align::Center));
		}
		At.Text(Floor, 1576.0f, 461.0f, igniteart::IgniteTx(13.0f, 600, pal::Muted, Align::Right));
	});
}

/** One shelf of the game floor: its label, the events on it (start order), what the label's sub-line says. */
struct IgniteShelf
{
	std::string Label, Sub;
	std::vector<size_t> Rows; // indices into the floor's rows
};

/** A cabinet as the floor draws it this frame: its row, its slot, the pointer on it, its entrance, and what its face says. */
struct IgniteCab
{
	const Access::ListRow* Row = nullptr;
	Rect R0;
	int Col = 0;
	Ui::ClickState St;
	bool Selected = false;
	float Enter = 1.0f;
	bool Late = false;
	int Mark = 0; // none, lock, LATE, REG
	std::string When;
	bool Rest = false; // not selected, not under the pointer, not in late registration
};

IgniteCab IgniteCabinetAt(IgniteCtx& X, const Access::ListRow& Row, const Rect& R0, int Shelf, int Col)
{
	IgniteCab Cab;
	Cab.Row = &Row;
	Cab.R0 = R0;
	Cab.Col = Col;
	Cab.St = X.UI.Clickable("ev:" + Row.E.Id, R0, !X.Preview);
	Cab.Selected = Row.E.Id == Access::EventId(X.RL);
	// Entrances run on the real clock, so a stream preview (a settled still, §1.11) draws them done.
	Cab.Enter = X.Preview ? 1.0f : IgniteEase((X.Now - std::max(X.U.ChipAt, Access::PageAt(X.RL)) - 0.025 * static_cast<double>(Col + Shelf * 2)) / 0.35);
	const net::LiveState& L = Row.L;
	Cab.Late = L.St == net::Status::LateReg;
	Cab.Mark = !Row.Joinable ? 1 : Cab.Late ? 2 : L.St == net::Status::Registering && L.StartsIn <= 60.0 ? 3 : 0;
	Cab.When = Cab.Late ? "LATE " + rlnet_detail::NetHms(L.LateRegLeft) : IgniteWhen(Row.E.Start, X.World);
	Cab.Rest = !Cab.Selected && !Cab.St.Hover && !Cab.Late;
	return Cab;
}

/** A cabinet's start (when Start isn't empty) and its lock or sticker, at (Ox, Oy). */
void IgniteCabinetFront(Canvas& At, const igniteart::IgniteTones& K, const std::string& Start, int Kind, float Ox, float Oy)
{
	using namespace igniteart;
	if (!Start.empty())
	{
		At.Text(Start, Ox + 56.0f, Oy + 24.0f, IgniteTx(12.5f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 96.0f));
	}
	if (Kind == 1)
	{
		rlnet_detail::NetLockIcon(At, Ox + 148.0f, Oy + 8.0f, 18.0f, Hex(K.Cream));
	}
	else if (Kind >= 2)
	{
		IgniteSticker(At, Ox + 156.0f, Oy + 18.0f, 17.0f, Kind == 2 ? "LATE" : "REG", Kind == 2 ? Hex(IgTang) : Hex(K.Sky), Hex(IgDarkInk), 8.5f, 14, 0.78f);
	}
}

/**
 * What a cabinet shows of its template (the body in the event's colour, the mark, the guarantee, the name, the buy-in) is the
 * same frame after frame: it replays from the cache, keyed by the template and the hour's tones (SPEC §10: PLAY drew at about
 * 1.6x RiverLine's lobby with every cabinet built afresh). A cabinet at rest caches its start time and its lock or REG sticker
 * with it, in the order they draw (they change on the hour, not the frame: G1_AUDIT M3).
 */
void IgniteCabinetFace(Canvas& C, const net::EventTemplate& T, const igniteart::IgniteTones& K, const IgniteCab& Cab, float X0, float Y0)
{
	using namespace igniteart;
	const bool Rest = Cab.Rest;
	const std::string Look = Rest ? "ignite:cabinet:" + T.Id + "|" + Cab.When + "|" + std::to_string(Cab.Mark) : "ignite:cabinet:" + T.Id;
	const uint64_t Key = (static_cast<uint64_t>(Fnv1a(Look)) << 8) ^ (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u);
	C.Cached(Key, X0, Y0, [&T, &K, &Cab, Rest](Canvas& At) {
		IgniteCabinetBody(At, {0.0f, 0.0f, 180.0f, 120.0f}, IgniteEventTint(T, K), false, 0.0f, K, false);
		IgniteEventMark(At, T, 30.0f, 27.0f, 18.0f, K);
		if (T.GtdCents > 0)
		{
			At.FillPolygon(IgniteStar(62.0f, 38.0f, 5.5f, 2.6f, 5), Hex(IgSun));
			At.Text(rlnet_detail::NetMoney(T.GtdCents), 70.0f, 42.0f, IgniteTx(11.0f, 800, Hex(IgSun)));
		}
		At.Text(T.Name, 14.0f, 74.0f, IgniteTx(14.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 152.0f));
		At.Text(net::BuyIn(T.BuyInCents), 14.0f, 101.0f, IgniteBi(23.0f, Hex(K.Cream)));
		At.FillRect({14.0f, 109.0f, 152.0f, 3.0f}, Rgba(255, 255, 255, 0.12f));
		if (Rest)
		{
			IgniteCabinetFront(At, K, Cab.When, Cab.Mark, 0.0f, 0.0f);
		}
	});
}

/** The field so far ("212 in") and its bar against where it will end up. */
void IgniteCabinetField(Canvas& C, const Access::ListRow& Row, const igniteart::IgniteTones& K, float X0, float Y0)
{
	C.Text(Grouped(Row.L.Entries) + " in", X0 + 166.0f, Y0 + 100.0f, igniteart::IgniteTx(12.0f, 800, Hex(igniteart::IgLime), Align::Right));
	const float Fill = Row.E.Entries > 0 ? static_cast<float>(Clamp01(static_cast<double>(Row.L.Entries) / static_cast<double>(Row.E.Entries))) : 0.0f;
	if (Fill > 0.0f)
	{
		C.FillRect({X0 + 14.0f, Y0 + 109.0f, std::max(3.0f, 152.0f * Fill), 3.0f}, Hex(K.Coral));
	}
}

/** A cabinet (180 x 120) drawn live: the event's emblem on a grape body, its start, name, buy-in, the field so far, a sticker. */
void IgniteCabinet(IgniteCtx& X, const IgniteCab& Cab)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const Access::ListRow& Row = *Cab.Row;
	const net::EventTemplate& T = net::ForEvent(Row.E.Id).TemplateOf(Row.E);
	const Ui::ClickState& St = Cab.St;
	const float Lift = St.Hover ? 4.0f : 0.0f;
	const float Enter = Cab.Enter;
	const float A0 = C.GetAlpha();
	C.SetAlpha(A0 * Enter);
	const Rect R{Cab.R0.X, Cab.R0.Y - Lift + (1.0f - Enter) * 12.0f, Cab.R0.W, Cab.R0.H};
	const float Breath = Cab.Selected ? IgnitePulse(X) : 0.0f;
	if (St.Hover && !Cab.Selected)
	{
		C.GlowRoundRect({R.X - 3.0f, R.Y - 3.0f, R.W + 6.0f, R.H + 6.0f}, 20.0f, IgniteHx(K.Coral, 0.28f), 12.0f);
	}
	if (Cab.Selected)
	{
		C.GlowRoundRect({R.X - 4.0f, R.Y - 4.0f, R.W + 8.0f, R.H + 8.0f}, 22.0f, IgniteA(Hex(K.Coral), 0.3f + 0.25f * Breath), 14.0f);
	}
	IgniteCabinetFace(C, T, K, Cab, R.X, R.Y);
	if (Cab.Selected || St.Hover)
	{
		C.StrokeRoundRect(R, 18.0f, Hex(K.Coral), Cab.Selected ? 2.0f : 1.5f);
	}
	if (!Cab.Rest)
	{
		C.Text(Cab.When, R.X + 56.0f, R.Y + 24.0f, IgniteTx(12.5f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 96.0f));
	}
	IgniteCabinetField(C, Row, K, R.X, R.Y);
	if (!Cab.Rest)
	{
		IgniteCabinetFront(C, K, std::string(), Cab.Mark, R.X, R.Y);
	}
	C.SetAlpha(A0);
}

void IgniteFloor(IgniteCtx& X, const std::vector<Access::ListRow>& Rows)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	IgniteUi& U = X.U;
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	// The floor's rows: the shared schedule, then the chip's filter.
	std::vector<size_t> Kept;
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		if (IgniteChipKeeps(U.Chip, Net.TemplateOf(Rows[I].E), Rows[I].L, X.S, Net))
		{
			Kept.push_back(I);
		}
	}
	// The shelves, in their order; an event may sit on more than one, Listed takes it once, where it first shows.
	const double Hour = std::fmod(X.World, net::MinutesPerDay) / 60.0;
	const double NextSix = std::floor((X.World - 6.0 * 60.0) / net::MinutesPerDay) * net::MinutesPerDay + 30.0 * 60.0; // the coming 6 AM
	std::vector<IgniteShelf> Shelves(7);
	Shelves[0].Label = "STARTING SOON";
	Shelves[1].Label = Hour >= 12.0 ? std::string("BIG GUARANTEES TONIGHT") : "BIG GUARANTEES \xC2\xB7 " + IgniteUpper(net::WeekdayName(net::DayOf(X.World), true));
	Shelves[2].Label = "FLIP & RUN \xC2\xB7 EVERY HOUR";
	Shelves[3].Label = "BOUNTIES & MYSTERY";
	Shelves[4].Label = "MY TICKETS";
	Shelves[5].Label = "CHEAP SHOTS";
	Shelves[6].Label = "TOMORROW MORNING";
	Chips Guaranteed = 0;
	for (const size_t I : Kept)
	{
		const net::EventTemplate& T = Net.TemplateOf(Rows[I].E);
		const net::LiveState& L = Rows[I].L;
		const double Start = Rows[I].E.Start;
		if (L.St == net::Status::LateReg || Start - X.World <= 90.0)
		{
			Shelves[0].Rows.push_back(I);
		}
		// The shelf holds what its label says: tonight's guarantees after noon, the day's before it (never tomorrow's).
		if (T.GtdCents > 0 && (Hour >= 12.0 ? IgniteTonight(Start, X.World) : net::DayOf(Start) == net::DayOf(X.World)))
		{
			Shelves[1].Rows.push_back(I);
			Guaranteed += T.GtdCents;
		}
		if (T.Fmt == net::Format::Flip)
		{
			Shelves[2].Rows.push_back(I);
		}
		if (T.Fmt == net::Format::Bounty || T.Fmt == net::Format::Mystery)
		{
			Shelves[3].Rows.push_back(I);
		}
		if (X.S.Life.TicketsFor(Net.TicketOf(T.Id)) > 0)
		{
			Shelves[4].Rows.push_back(I);
		}
		if (T.BuyInCents <= 220)
		{
			Shelves[5].Rows.push_back(I);
		}
		if (Start >= NextSix && Start < NextSix + 6.0 * 60.0)
		{
			Shelves[6].Rows.push_back(I);
		}
	}
	Shelves[1].Sub = Guaranteed > 0 ? rlnet_detail::NetMoney(Guaranteed) + " guaranteed" : std::string();
	std::vector<IgniteShelf> Shown;
	std::vector<int> Slot; // each shown shelf's index into U.ShelfX
	for (size_t I = 0; I < Shelves.size(); ++I)
	{
		if (!Shelves[I].Rows.empty())
		{
			if (Shelves[I].Sub.empty())
			{
				Shelves[I].Sub = std::to_string(Shelves[I].Rows.size()) + (Shelves[I].Rows.size() == 1 ? " event" : " events");
			}
			Shown.push_back(Shelves[I]);
			Slot.push_back(static_cast<int>(I));
		}
	}
	// Listed: shelf, then tile (arrow keys walk the cabinets); "always select something" (NetLobby's rule).
	std::vector<std::string>& Listed = Access::Listed(X.RL);
	Listed.clear();
	for (const IgniteShelf& Sh : Shown)
	{
		for (const size_t I : Sh.Rows)
		{
			if (std::find(Listed.begin(), Listed.end(), Rows[I].E.Id) == Listed.end())
			{
				Listed.push_back(Rows[I].E.Id);
			}
		}
	}
	// The chips, and the count of what's on the shelves (an open event no shelf holds, a GTD tomorrow afternoon, isn't on
	// the floor, so it isn't counted).
	IgniteChips(X, static_cast<int>(Listed.size()));
	const std::string& Sel = Access::EventId(X.RL);
	if (std::find(Listed.begin(), Listed.end(), Sel) == Listed.end() && !Listed.empty() && !U.Sheet)
	{
		std::string Pick;
		for (const std::string& Id : Listed)
		{
			for (const size_t I : Kept)
			{
				if (Rows[I].E.Id == Id && Pick.empty() && Rows[I].Joinable && rlsites_detail::SitesBalance(X.S, net::SiteId::Ignite) >= Net.TemplateOf(Rows[I].E).BuyInCents)
				{
					Pick = Id;
				}
			}
		}
		Access::TakeEventId(X.RL, Pick.empty() ? Listed.front() : Pick);
	}
	// The selection moved (an arrow key, a sign): scroll its shelf to it, once.
	if (Access::TakeScrollToSelected(X.RL) || U.FollowId != Access::EventId(X.RL))
	{
		U.FollowId = Access::EventId(X.RL);
		for (size_t S0 = 0; S0 < Shown.size(); ++S0)
		{
			const auto Where = std::find_if(Shown[S0].Rows.begin(), Shown[S0].Rows.end(), [&](size_t I) { return Rows[I].E.Id == U.FollowId; });
			if (Where != Shown[S0].Rows.end())
			{
				const float Col = static_cast<float>(Where - Shown[S0].Rows.begin());
				float& Goal = U.ShelfGoal[static_cast<size_t>(Slot[S0])];
				if (Col < Goal || Col > Goal + 7.0f)
				{
					Goal = std::max(0.0f, Col - 3.0f);
				}
				const float Top = static_cast<float>(S0) * 156.0f;
				if (Top < U.FloorGoal || Top > U.FloorGoal + 156.0f)
				{
					U.FloorGoal = Top;
				}
				break;
			}
		}
	}
	// The floor: shelves at a pitch of 156, the wheel scrolling more of them into view.
	const Rect Floor{0.0f, 484.0f, 1600.0f, 348.0f};
	if (X.UI.Hover(Floor) && X.UI.Ptr.Wheel != 0.0f)
	{
		U.FloorGoal += X.UI.Ptr.Wheel > 0.0f ? 156.0f : -156.0f;
	}
	const float MaxFloor = std::max(0.0f, static_cast<float>(Shown.size()) * 156.0f - 312.0f);
	U.FloorGoal = std::min(std::max(U.FloorGoal, 0.0f), MaxFloor);
	U.FloorScroll += (U.FloorGoal - U.FloorScroll) * IgniteFollow(X.Dt, 12.0);
	if (std::fabs(U.FloorGoal - U.FloorScroll) < 0.4f || X.Preview)
	{
		U.FloorScroll = U.FloorGoal;
	}
	if (Shown.empty())
	{
		C.Text(IgniteNoneOpen, 800.0f, 600.0f, igniteart::IgniteTx(17.0f, 600, pal::Muted, Align::Center));
		return;
	}
	C.PushClip(Floor);
	for (size_t S0 = 0; S0 < Shown.size(); ++S0)
	{
		const float Y = 500.0f + static_cast<float>(S0) * 156.0f - U.FloorScroll;
		if (Y > Floor.Y + Floor.H + 20.0f || Y + 140.0f < Floor.Y)
		{
			continue;
		}
		const IgniteShelf& Sh = Shown[S0];
		const size_t Sl = static_cast<size_t>(Slot[S0]);
		const float Lw = C.Text(Sh.Label, 24.0f, Y, igniteart::IgniteBi(16.0f, Hex(K.Cream)));
		C.Text(Sh.Sub, 34.0f + Lw, Y, igniteart::IgniteTx(12.0f, 600, pal::Muted));
		const float Count = static_cast<float>(Sh.Rows.size());
		const float MaxX = std::max(0.0f, Count - 8.0f);
		// The arrows page the shelf sideways, four cabinets a press.
		for (int Dir = 0; Dir < 2; ++Dir)
		{
			const float Ax = 1540.0f + static_cast<float>(Dir) * 30.0f;
			const bool Can = Dir == 0 ? U.ShelfGoal[Sl] > 0.0f : U.ShelfGoal[Sl] < MaxX;
			const Ui::ClickState St = X.UI.Clickable("shelf" + std::to_string(Sl) + (Dir == 0 ? "l" : "r"), {Ax - 13.0f, Y - 18.0f, 26.0f, 26.0f}, Can && !X.Preview);
			C.FillCircle(Ax, Y - 5.0f, 12.0f, IgniteHx(St.Hover ? K.Panel : K.Well));
			C.StrokeArc(Ax, Y - 5.0f, 12.0f, 0.0f, 2.0f * Pi, IgniteHx(K.Rim), 1.2f);
			rlnet_detail::NetChevron(C, Ax, Y - 5.0f, 12.0f, Dir == 0 ? 1 : 0, Can ? Hex(K.Cream) : IgniteHx(K.Cream, 0.3f));
			if (St.Clicked)
			{
				U.ShelfGoal[Sl] = std::min(MaxX, std::max(0.0f, U.ShelfGoal[Sl] + (Dir == 0 ? -4.0f : 4.0f)));
			}
		}
		U.ShelfGoal[Sl] = std::min(U.ShelfGoal[Sl], MaxX);
		const float Before = U.ShelfX[Sl];
		U.ShelfX[Sl] += (U.ShelfGoal[Sl] - U.ShelfX[Sl]) * IgniteFollow(X.Dt, 12.0);
		if (std::fabs(U.ShelfGoal[Sl] - U.ShelfX[Sl]) < 0.01f || X.Preview)
		{
			U.ShelfX[Sl] = U.ShelfGoal[Sl];
		}
		// A cabinet sliding past its slot ticks (IgTick: the reel's sound, SPEC §2.3).
		if (std::floor(Before) != std::floor(U.ShelfX[Sl]) && !X.Preview)
		{
			X.S.UiSound(SoundId::IgTick, 0.3f);
		}
		C.PushClip({16.0f, Y - 8.0f, 1568.0f, 140.0f});
		const float Top = Y + 8.0f;
		std::vector<IgniteCab> Cabs;
		for (size_t Ci = 0; Ci < Sh.Rows.size(); ++Ci)
		{
			const float Cx = 24.0f + (static_cast<float>(Ci) - U.ShelfX[Sl]) * 196.0f;
			if (Cx < -190.0f || Cx > 1600.0f)
			{
				continue;
			}
			const Access::ListRow& Row = Rows[Sh.Rows[Ci]];
			const float Seen = std::min(Top + 120.0f, Floor.Y + Floor.H) - std::max(Top, Floor.Y);
			if (Seen < 70.0f)
			{
				// A shelf peeking in at the floor's edge: the cabinets' tops only (their faces would be clipped anyway).
				if (Seen > 0.0f)
				{
					C.FillRoundRect({Cx, Top, 180.0f, 120.0f}, 18.0f, Mix(Hex(K.CabTop), igniteart::IgniteEventTint(net::ForEvent(Row.E.Id).TemplateOf(Row.E), K), 0.3f));
				}
				continue;
			}
			Cabs.push_back(IgniteCabinetAt(X, Row, {Cx, Top, 180.0f, 120.0f}, static_cast<int>(Sl), static_cast<int>(Ci)));
		}
		// A shelf that isn't moving draws its settled cabinets (not selected, not under the pointer) as one piece from the
		// Boardwalk pool, keyed by what each shows: a field that grows takes a new piece and the pool lets the old one go
		// (G1_AUDIT M3). A cabinet in late registration is in the piece by its face alone: its countdown and its field (which
		// tick) draw live over it, then its LATE sticker from a second piece, in the order a cabinet always drew them.
		const bool Still = U.ShelfX[Sl] == U.ShelfGoal[Sl] && U.FloorScroll == U.FloorGoal;
		const auto Pieced = [Still](const IgniteCab& Cab) { return Still && !Cab.Selected && !Cab.St.Hover && Cab.Enter >= 1.0f; };
		const uint64_t Tone = (static_cast<uint64_t>(K.Mood) << 1) ^ (K.Night ? 1u : 0u);
		if (Still)
		{
			std::string Piece = "ignite:shelf|" + std::to_string(Sl) + "|" + std::to_string(std::lround(U.ShelfX[Sl]));
			std::string Late = "ignite:shelf-late|" + std::to_string(Sl) + "|" + std::to_string(std::lround(U.ShelfX[Sl]));
			bool AnyLate = false;
			for (const IgniteCab& Cab : Cabs)
			{
				if (Pieced(Cab))
				{
					Piece += "|" + std::to_string(Cab.Col) + ":" + Cab.Row->E.Id + (Cab.Late ? std::string(":late") : ":" + Cab.When + ":" + std::to_string(Cab.Mark) + ":" + std::to_string(Cab.Row->L.Entries));
					if (Cab.Late)
					{
						Late += "|" + std::to_string(Cab.Col) + ":" + std::to_string(Cab.Mark);
						AnyLate = true;
					}
				}
			}
			C.Cached((static_cast<uint64_t>(Fnv1a(Piece)) << 8) ^ Tone, 0.0f, Top, [&Cabs, &K, &Pieced](Canvas& At) {
				for (const IgniteCab& Cab : Cabs)
				{
					if (Pieced(Cab))
					{
						IgniteCabinetFace(At, net::ForEvent(Cab.Row->E.Id).TemplateOf(Cab.Row->E), K, Cab, Cab.R0.X, 0.0f);
						if (!Cab.Late)
						{
							IgniteCabinetField(At, *Cab.Row, K, Cab.R0.X, 0.0f);
						}
					}
				}
			}, Canvas::CachePool::Boardwalk);
			if (AnyLate)
			{
				for (const IgniteCab& Cab : Cabs)
				{
					if (Pieced(Cab) && Cab.Late)
					{
						C.Text(Cab.When, Cab.R0.X + 56.0f, Top + 24.0f, igniteart::IgniteTx(12.5f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 96.0f));
						IgniteCabinetField(C, *Cab.Row, K, Cab.R0.X, Top);
					}
				}
				C.Cached((static_cast<uint64_t>(Fnv1a(Late)) << 8) ^ Tone, 0.0f, Top, [&Cabs, &K, &Pieced](Canvas& At) {
					for (const IgniteCab& Cab : Cabs)
					{
						if (Pieced(Cab) && Cab.Late)
						{
							IgniteCabinetFront(At, K, std::string(), Cab.Mark, Cab.R0.X, 0.0f);
						}
					}
				}, Canvas::CachePool::Boardwalk);
			}
		}
		for (const IgniteCab& Cab : Cabs)
		{
			if (!Pieced(Cab))
			{
				IgniteCabinet(X, Cab);
			}
		}
		C.PopClip();
		for (const IgniteCab& Cab : Cabs)
		{
			if (Cab.St.Clicked)
			{
				IgniteOpenSheet(X.RL, Cab.Row->E.Id, X.Now);
			}
		}
	}
	C.PopClip();
	// The floor fades out at its foot, clear of the raised button (pages end at 832, SPEC §2.4 keep-out).
	C.FillRect({0.0f, 800.0f, 1600.0f, 34.0f}, Paint::Linear({0.0f, 800.0f}, {0.0f, 834.0f}, IgniteHx(K.Plum, 0.0f), IgniteHx(K.Plum, 1.0f)));
}

// ------------------------------------------------------------------ the event sheet (SPEC §2.4 B)

const Rect IgniteSheetRect{24.0f, 300.0f, 1552.0f, 570.0f};

/** A label over a value (the sheet's facts). */
void IgniteFact(Canvas& C, const std::string& Label, const std::string& Value, float X, float Y, const Color& Col, float MaxW)
{
	C.Text(Label, X, Y, igniteart::IgniteTx(10.0f, 800, pal::Muted));
	C.Text(Value, X, Y + 22.0f, igniteart::IgniteTx(17.0f, 700, Col, Align::Left, Baseline::Alphabetic, MaxW));
}

void IgniteSheet(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const net::Network& Net = net::ForEvent(Access::EventId(X.RL));
	net::EventInstance E;
	if (!Net.FindInstance(Access::EventId(X.RL), E))
	{
		X.U.Sheet = false;
		return;
	}
	const net::EventTemplate& T = Net.TemplateOf(E);
	const net::LiveState L = Net.Live(E, X.World);
	const float Rise = X.Preview ? 1.0f : IgniteBack((X.Now - X.U.SheetAt) / 0.35);
	const float Fade = X.Preview ? 1.0f : IgniteEase((X.Now - X.U.SheetAt) / 0.2);
	const float A0 = C.GetAlpha();
	// The scrim over the floor, then the sheet rising from the dock.
	C.SetAlpha(A0 * Fade);
	C.FillRect({0.0f, 72.0f, 1600.0f, 890.0f}, IgniteHx(IgNight, 0.78f));
	C.Save();
	C.Translate(0.0f, (1.0f - Rise) * 120.0f);
	const Rect S0 = IgniteSheetRect;
	C.GlowRoundRect({S0.X, S0.Y + 10.0f, S0.W, S0.H}, 24.0f, Rgba(0, 0, 0, 0.5f), 30.0f);
	C.FillRoundRect(S0, 24.0f, Paint::Linear({0.0f, S0.Y}, {0.0f, S0.Y + S0.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
	IgniteNeonRoundRect(C, S0, 24.0f, IgniteHx(K.Coral, 0.7f), 1.5f, 6.0f);
	C.FillRoundRect({S0.X, S0.Y, S0.W, 90.0f}, 24.0f, Paint::Linear({S0.X, 0.0f}, {S0.X + S0.W, 0.0f}, IgniteHx(IgPink, 0.14f), IgniteHx(IgViolet, 0.06f)));

	// Identity: the emblem in a starburst, the name, the buy-in split, the guarantee and the field, the blurb, WHO'S IN.
	{
		const float Ix = 48.0f;
		C.FillPolygon(IgniteStar(Ix + 66.0f, 394.0f, 66.0f, 52.0f, 16), IgniteHx(K.Coral, 0.22f));
		eventart::Emblem(C, T, Ix + 66.0f, 394.0f, 96.0f, X.Tm);
		// The name in BlackItalic 32 (SPEC §2.4 B), fitted down to 22 on one line, measured in its own face (g1-audit).
		float Ns = 32.0f;
		while (Ns > 22.0f && C.Measure(T.Name, Ns, Font::BlackItalic) > 370.0f)
		{
			Ns -= 1.0f;
		}
		C.Text(T.Name, Ix + 148.0f, 376.0f, IgniteBi(Ns, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 370.0f));
		const Chips Fee = Net.Definition().Rake.Fee(T.BuyInCents);
		const std::string Split = T.BuyInCents == 0 ? std::string("Free to play") : net::BuyIn(T.BuyInCents) + " = " + Money(T.BuyInCents - Fee) + " + " + Money(Fee);
		C.Text(Split, Ix + 148.0f, 440.0f, IgniteTx(15.0f, 700, Hex(IgSun)));
		C.Text(rlnet_detail::NetFormat(T) + " \xC2\xB7 " + T.Speed + " \xC2\xB7 " + std::to_string(T.TableSize) + "-max", Ix, 484.0f, IgniteTx(13.0f, 800, Hex(K.Sky)));
		IgniteFact(C, T.GtdCents > 0 ? "GUARANTEED" : "PRIZE POOL", rlnet_detail::NetMoney(T.GtdCents > 0 ? std::max(T.GtdCents, L.Pool) : L.Pool), Ix, 516.0f, Hex(IgSun), 160.0f);
		IgniteFact(C, "ENTRIES", Grouped(L.Entries) + (L.St == net::Status::Registering || L.St == net::Status::LateReg ? " of ~" + Grouped(E.Entries) : std::string()), Ix + 180.0f, 516.0f, Hex(K.Cream), 160.0f);
		IgniteFact(C, "STARTS", IgniteWhen(E.Start, X.World), Ix + 350.0f, 516.0f, Hex(K.Cream), 170.0f);
		std::string Blurb = T.Blurb;
		if (Blurb.empty())
		{
			Blurb = "A " + net::BuyIn(T.BuyInCents) + " " + rlnet_detail::NetLower(rlnet_detail::NetFormat(T)) + " with " + (T.GtdCents > 0 ? rlnet_detail::NetMoney(T.GtdCents) + " guaranteed" : std::string("a pool built by its entries")) +
				". " + T.Speed + " structure, " + std::to_string(static_cast<int>(T.LevelMinutes)) + "-minute levels; about " + Grouped(E.Entries) + " players expected.";
		}
		rlnet_detail::NetParagraph(C, Blurb, Ix, 582.0f, 500.0f, 14.0f, 500, IgniteHx(K.Cream, 0.82f), 20.0f, 3);
		C.Text("WHO'S IN", Ix, 670.0f, IgniteBi(15.0f, Hex(IgPink)));
		std::vector<int> Who = X.S.Living().Registered(E.Id);
		const world::World& W = X.S.Living();
		std::stable_sort(Who.begin(), Who.end(), [&W](int A, int B) { return W.Get(A)->RepOf(world::Rep::Overall) > W.Get(B)->RepOf(world::Rep::Overall); });
		if (Who.empty())
		{
			C.Text(IgniteNoRegulars, Ix, 696.0f, IgniteTx(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, 500.0f));
		}
		for (size_t I = 0; I < Who.size() && I < 8; ++I)
		{
			const world::Npc* N = W.Get(Who[I]);
			const float Wx = Ix + static_cast<float>(I % 2) * 256.0f;
			const float Wy = 698.0f + static_cast<float>(I / 2) * 34.0f;
			C.FillRoundRect({Wx, Wy - 14.0f, 244.0f, 28.0f}, 14.0f, IgniteHx(K.Well));
			const AvatarSpec Pic = AvatarFor(N->Name);
			DrawAvatar(C, Wx + 14.0f, Wy, 11.0f, Pic);
			C.Text(N->Name, Wx + 32.0f, Wy + 5.0f, IgniteTx(13.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 200.0f));
		}
		if (Who.size() > 8)
		{
			C.Text("+" + std::to_string(Who.size() - 8) + " more regulars", Ix, 840.0f, IgniteTx(12.0f, 700, pal::Muted));
		}
	}

	// The detail tabs: Overview, Structure, Payouts, Players (today's EventPanel data, in Ignite's look).
	{
		const Rect D{592.0f, 324.0f, 500.0f, 508.0f};
		C.FillRoundRect(D, 20.0f, IgniteHx(K.Plum, 0.55f));
		C.StrokeRoundRect(D, 20.0f, IgniteHx(K.Rim), 1.0f);
		const char* const Tabs[4] = {"Overview", "Structure", "Payouts", "Players"};
		float Tx = D.X + 16.0f;
		for (int I = 0; I < 4; ++I)
		{
			const float Tw = C.Measure(Tabs[I], 13.5f, 800) + 28.0f;
			const Rect R{Tx, D.Y + 16.0f, Tw, 32.0f};
			const Ui::ClickState St = X.UI.Clickable("sheettab" + std::to_string(I), R, !X.Preview);
			if (St.Clicked)
			{
				X.U.SheetTab = I;
			}
			const bool On = X.U.SheetTab == I;
			IgnitePill(C, R, On ? Hex(K.Coral) : IgniteHx(St.Hover ? K.Panel : K.Well), IgniteHx(K.Rim), On ? 0.0f : 1.0f);
			C.Text(Tabs[I], R.X + Tw / 2.0f, R.Y + 21.0f, IgniteTx(13.5f, 800, On ? IgniteHx(0x2a0d14) : Hex(K.Cream), Align::Center));
			Tx += Tw + 8.0f;
		}
		const float Bx = D.X + 24.0f;
		float By = D.Y + 84.0f;
		const float Bw = D.W - 48.0f;
		if (X.U.SheetTab == 0)
		{
			std::string Label;
			std::string Value;
			switch (L.St)
			{
			case net::Status::Announced: Label = "REGISTRATION OPENS IN"; Value = rlnet_detail::NetHms(L.RegOpensIn); break;
			case net::Status::Registering: Label = "STARTS IN"; Value = rlnet_detail::NetHms(L.StartsIn); break;
			case net::Status::LateReg: Label = "LATE REGISTRATION CLOSES IN"; Value = rlnet_detail::NetHms(L.LateRegLeft); break;
			case net::Status::Running: Label = "RUNNING \xC2\xB7 LEVEL " + std::to_string(L.Level); Value = Grouped(L.Left) + " left"; break;
			case net::Status::FinalTable: Label = "FINAL TABLE \xC2\xB7 LEVEL " + std::to_string(L.Level); Value = Grouped(L.Left) + " left"; break;
			case net::Status::Finished: Label = "FINISHED"; Value = net::TimeLabel(E.Start + E.Duration); break;
			}
			C.Text(Label, Bx, By, IgniteTx(11.0f, 800, pal::Muted));
			C.Text(Value, Bx, By + 46.0f, IgniteBi(40.0f, L.St == net::Status::LateReg ? Hex(IgLime) : Hex(K.Cream)));
			By += 90.0f;
			const int Paid = std::max(1, static_cast<int>(std::round(static_cast<double>(E.Entries) * 0.15)));
			IgniteFact(C, "PLACES PAID", Grouped(Paid), Bx, By, Hex(K.Cream), 200.0f);
			IgniteFact(C, "STARTING STACK", Grouped(T.StartStack), Bx + 230.0f, By, Hex(K.Cream), 200.0f);
			By += 62.0f;
			IgniteFact(C, "LATE REG", L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg ? "Until " + net::TimeLabel(E.Start + E.LateReg) : std::string("Closed"), Bx, By, Hex(K.Cream), 200.0f);
			IgniteFact(C, "LEVELS", std::to_string(static_cast<int>(T.LevelMinutes)) + " minutes", Bx + 230.0f, By, Hex(K.Cream), 200.0f);
			By += 62.0f;
			if (!T.Seats.empty())
			{
				C.FillRoundRect({Bx, By, Bw, 40.0f}, 12.0f, IgniteHx(igniteart::IgSun, 0.1f));
				C.Text("Prizes: " + T.Seats, Bx + 14.0f, By + 26.0f, IgniteTx(14.0f, 700, Hex(igniteart::IgSun), Align::Left, Baseline::Alphabetic, Bw - 28.0f));
				By += 52.0f;
			}
			std::string Lock;
			if (!Net.Joinable(E, &Lock, X.S.Unlocks()))
			{
				C.FillRoundRect({Bx, By, Bw, 54.0f}, 12.0f, IgniteHx(K.Well));
				rlnet_detail::NetLockIcon(C, Bx + 14.0f, By + 16.0f, 22.0f, pal::Muted);
				C.Text("Locked", Bx + 48.0f, By + 23.0f, IgniteTx(14.0f, 800, Hex(K.Cream)));
				C.Text(Lock, Bx + 48.0f, By + 42.0f, IgniteTx(13.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, Bw - 60.0f));
			}
		}
		else if (X.U.SheetTab == 1)
		{
			const int LateLevels = static_cast<int>(std::ceil(E.LateReg / std::max(1.0, T.LevelMinutes)));
			const std::pair<std::string, std::string> Rows[6] = {
				{"Game", std::string(T.Omaha ? "PL Omaha" : "NL Hold'em") + " \xC2\xB7 " + std::to_string(T.TableSize) + "-max"},
				{"Speed", T.Speed + " \xC2\xB7 " + std::to_string(static_cast<int>(T.LevelMinutes)) + "-minute levels"},
				{"Starting stack", Grouped(T.StartStack) + " chips"},
				{"Blinds", "Level 1: " + std::string(T.StartStack > 10000 ? "100/200" : "50/100")},
				{"Late registration", net::Countdown(E.LateReg) + " (to level " + std::to_string(LateLevels + 1) + ")"},
				{"Estimated finish", net::TimeLabel(E.Start + E.Duration) + " \xC2\xB7 " + net::Countdown(E.Duration) + " of poker"},
			};
			for (const auto& Row : Rows)
			{
				C.FillRoundRect({Bx, By - 22.0f, Bw, 40.0f}, 12.0f, IgniteHx(K.Well, 0.7f));
				C.Text(Row.first, Bx + 14.0f, By + 3.0f, IgniteTx(13.5f, 600, pal::Muted));
				C.Text(Row.second, Bx + Bw - 14.0f, By + 3.0f, IgniteTx(14.0f, 700, Hex(K.Cream), Align::Right, Baseline::Alphabetic, Bw - 170.0f));
				By += 48.0f;
			}
		}
		else if (X.U.SheetTab == 2)
		{
			const std::vector<Chips> Pay = Net.Payouts(E);
			const float Top = Pay.empty() ? 1.0f : static_cast<float>(Pay.front());
			const Color Medals[3] = {Hex(igniteart::IgSun), Hex(0xd9d4e8), Hex(0xe0905a)};
			for (size_t I = 0; I < Pay.size() && I < 9; ++I)
			{
				const float Y = By - 12.0f + static_cast<float>(I) * 36.0f;
				if (I < 3)
				{
					C.FillCircle(Bx + 12.0f, Y + 8.0f, 11.0f, Medals[I]);
					C.Text(std::to_string(I + 1), Bx + 12.0f, Y + 8.5f, IgniteTx(11.0f, 900, IgniteHx(0x2a1640), Align::Center, Baseline::Middle));
				}
				else
				{
					C.Text(Ordinal(static_cast<int>(I) + 1), Bx, Y + 13.0f, IgniteTx(13.0f, 700, pal::Muted));
				}
				const float W = (Bw - 190.0f) * static_cast<float>(Pay[I]) / Top;
				C.FillRoundRect({Bx + 40.0f, Y + 3.0f, std::max(4.0f, W), 12.0f}, 6.0f, Paint::Linear({Bx + 40.0f, 0.0f}, {Bx + Bw - 150.0f, 0.0f}, Hex(K.Coral), Hex(igniteart::IgPink)));
				C.Text(Money(Pay[I]), Bx + Bw, Y + 14.0f, IgniteBi(16.0f, I == 0 ? Hex(igniteart::IgSun) : Hex(K.Cream), Align::Right));
			}
			C.Text(Grouped(static_cast<int64_t>(Pay.size())) + " places paid \xC2\xB7 min cash " + (Pay.empty() ? std::string("-") : Money(Pay.back())), Bx, D.Y + D.H - 20.0f, IgniteTx(12.5f, 600, pal::Muted));
		}
		else
		{
			std::vector<int> Who = X.S.Living().Registered(E.Id);
			const world::World& W = X.S.Living();
			std::stable_sort(Who.begin(), Who.end(), [&W](int A, int B) { return W.Get(A)->RepOf(world::Rep::Overall) > W.Get(B)->RepOf(world::Rep::Overall); });
			C.Text(Who.empty() ? std::string("No regulars registered yet") : "Regulars registered \xC2\xB7 " + std::to_string(Who.size()), Bx, By - 6.0f, IgniteTx(13.0f, 700, pal::Muted));
			for (size_t I = 0; I < Who.size() && I < 10; ++I)
			{
				const world::Npc* N = W.Get(Who[I]);
				const float Y = By + 14.0f + static_cast<float>(I) * 33.0f;
				const AvatarSpec Pic = AvatarFor(N->Name);
				DrawAvatar(C, Bx + 12.0f, Y + 10.0f, 12.0f, Pic);
				C.Text(N->Name, Bx + 34.0f, Y + 15.0f, IgniteTx(14.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, Bw - 200.0f));
				const bool Fresh = N->Came != world::Arrival::None && net::DayOf(X.World) - N->Arrived <= 30;
				C.Text(Fresh ? std::string("Joined ") + net::DateLabel(N->Arrived) : std::string(world::IdentityName(N->Is)), Bx + Bw, Y + 15.0f,
					IgniteTx(12.0f, 600, Fresh ? Hex(K.Coral) : pal::Muted, Align::Right, Baseline::Alphabetic, 160.0f));
			}
		}
	}

	// Register: the shared RegisterBlock (every state, the clause's two presses) in Ignite's look, and the money beside it.
	{
		const Rect Rg{1116.0f, 324.0f, 436.0f, 522.0f};
		C.FillRoundRect(Rg, 20.0f, IgniteHx(K.Plum, 0.55f));
		C.StrokeRoundRect(Rg, 20.0f, IgniteHx(K.Rim), 1.0f);
		const Chips Bal = rlsites_detail::SitesBalance(X.S, net::SiteId::Ignite);
		const int Tickets = X.S.Life.TicketsFor(Net.TicketOf(T.Id));
		C.Text("YOUR IGNITE BALANCE", Rg.X + 24.0f, Rg.Y + 40.0f, IgniteTx(11.0f, 800, pal::Muted));
		C.Text(Money(Bal), Rg.X + 24.0f, Rg.Y + 90.0f, IgniteBi(44.0f, Hex(igniteart::IgSun)));
		C.Text("BUY-IN", Rg.X + 24.0f, Rg.Y + 134.0f, IgniteTx(11.0f, 800, pal::Muted));
		C.Text(T.BuyInCents == 0 ? std::string("Free") : net::BuyIn(T.BuyInCents), Rg.X + 24.0f, Rg.Y + 166.0f, IgniteBi(28.0f, Hex(K.Cream)));
		if (Tickets > 0)
		{
			C.Text(std::to_string(Tickets) + (Tickets == 1 ? " ticket held" : " tickets held"), Rg.X + Rg.W - 24.0f, Rg.Y + 166.0f, IgniteTx(14.0f, 800, Hex(igniteart::IgSun), Align::Right));
		}
		else if (T.BuyInCents > 0)
		{
			C.Text("after: " + Money(Bal - T.BuyInCents), Rg.X + Rg.W - 24.0f, Rg.Y + 166.0f, IgniteTx(14.0f, 700, Bal >= T.BuyInCents ? pal::Muted : pal::Red, Align::Right));
		}
		rlnet_detail::NetParagraph(C, IgniteStaysHere, Rg.X + 24.0f, Rg.Y + 214.0f, Rg.W - 48.0f, 13.5f, 600, pal::Muted, 19.0f, 2);
		if (const stake::Deal* Package = stake::ActivePackage(X.S.Life.Stakes))
		{
			rlnet_detail::NetParagraph(C, rlsites_detail::SitesFill(rlsites_detail::SitesOwnMoney, {{"Backer", stake::BackerName(*Package)}}), Rg.X + 24.0f, Rg.Y + 270.0f, Rg.W - 48.0f, 13.0f, 700,
				Hex(igniteart::IgTang), 18.0f, 2);
		}
		Access::RegisterBlock(X.RL, E, L, {Rg.X + 24.0f, Rg.Y + Rg.H - 104.0f, Rg.W - 48.0f, 72.0f}, X.Now);
	}

	// Close.
	{
		const Rect Xr{1520.0f, 312.0f, 40.0f, 40.0f};
		const Ui::ClickState St = X.UI.Clickable("sheetx", Xr, !X.Preview);
		C.FillCircle(1540.0f, 332.0f, 18.0f, IgniteHx(St.Hover ? K.Coral : K.Well));
		C.StrokeArc(1540.0f, 332.0f, 18.0f, 0.0f, 2.0f * Pi, IgniteHx(K.Rim), 1.2f);
		C.StrokePolyline({{1533.0f, 325.0f}, {1547.0f, 339.0f}}, false, Hex(K.Cream), 2.2f, true);
		C.StrokePolyline({{1547.0f, 325.0f}, {1533.0f, 339.0f}}, false, Hex(K.Cream), 2.2f, true);
		if (St.Clicked)
		{
			X.U.Sheet = false;
			X.S.ConfirmRegister = false;
		}
	}
	C.Restore();
	C.SetAlpha(A0);
}

// ------------------------------------------------------------------ PLAY

void IgnitePlay(IgniteCtx& X)
{
	// The carpet under the floor: a faint scatter of neon confetti on the plum ground (keyed, never random), still, so it
	// replays from the Boardwalk pool by the hour's tones (G1_AUDIT M3).
	{
		const igniteart::IgniteTones& K = X.K;
		X.C.Cached((static_cast<uint64_t>(Fnv1a("ignite:carpet")) << 32) ^ (static_cast<uint64_t>(K.Plum) << 8) ^ static_cast<uint64_t>(K.Sky), 0.0f, 0.0f, [&K](Canvas& C) {
			C.FillRect({0.0f, 352.0f, 1600.0f, 610.0f}, IgniteHx(K.Plum));
			C.GlowRoundRect({-100.0f, 330.0f, 1800.0f, 80.0f}, 40.0f, IgniteHx(igniteart::IgViolet, 0.08f), 60.0f);
			const uint32_t Cols[5] = {igniteart::IgPink, igniteart::IgViolet, K.Sky, igniteart::IgLime, igniteart::IgSun};
			for (uint32_t I = 0; I < 30; ++I)
			{
				const float Px = static_cast<float>(igniteart::IgniteMix(I * 2654435761u + 17u) % 1600u);
				const float Py = 356.0f + static_cast<float>(igniteart::IgniteMix(I * 2246822519u + 29u) % 600u);
				C.FillPolygon({{Px, Py - 4.0f}, {Px + 3.0f, Py}, {Px, Py + 4.0f}, {Px - 3.0f, Py}}, IgniteHx(Cols[I % 5], 0.08f));
			}
		}, Canvas::CachePool::Boardwalk);
	}
	IgniteBoardwalk(X);
	IgniteToday(X);
	IgniteFloor(X, Access::ScheduleRows(X.RL));
}

void IgniteBoards(IgniteCtx& X);
void IgniteBuzz(IgniteCtx& X);
void IgniteWallet(IgniteCtx& X);
void IgniteCard(IgniteCtx& X);

void IgnitePage(IgniteCtx& X, RiverLine::Page Page)
{
	switch (Page)
	{
	case RiverLine::Page::Promotions: IgniteRewards(X.RL, X.U, X.K, X.Now, X.Preview); break;
	case RiverLine::Page::Leaderboards: IgniteBoards(X); break;
	case RiverLine::Page::News: IgniteBuzz(X); break;
	case RiverLine::Page::Cashier: IgniteWallet(X); break;
	case RiverLine::Page::Profile: IgniteCard(X); break;
	default: IgnitePlay(X); break;
	}
}

/** Ignite's lobby: the page under the marquee and over the dock (the dock's pill and raised button keep their pointer). */
void IgniteLobby(IgniteCtx& X)
{
	RiverLine& RL = X.RL;
	using Pg = RiverLine::Page;
	// A page Ignite hasn't got goes to PLAY (RiverLine's own, RunItTwice's); FUSE's booth is G2: until FUSE lights, its page
	// is PLAY on Flip & Run, as the centre button opens it.
	Pg Page = rlsites_detail::SitesPageHome(net::SiteId::Ignite, Access::Shown(RL));
	if (Page == Pg::Fuse)
	{
		Page = Pg::Lobby;
		X.U.Chip = 2;
		X.U.ChipAt = X.Now;
	}
	if (Page == Pg::Series && !IgniteWildfire(net::Site(net::SiteId::Ignite), X.World))
	{
		// SERIES shows while Wildfire runs (the Lighthouse's sign opens it); the shared series page outside one.
	}
	Access::SetPage(RL, Page);
	if (Page != Pg::Lobby)
	{
		X.U.Sheet = false;
	}
	const bool SheetUp = X.U.Sheet && !X.Preview;
	if (!SheetUp && X.S.ConfirmRegister && !Access::EventId(RL).empty() && Page == Pg::Lobby && !X.Preview)
	{
		// A confirm step opened elsewhere (a featured link, a script) lives in the sheet.
		IgniteOpenSheet(RL, Access::EventId(RL), X.Now);
	}
	const bool Sheet = X.U.Sheet && !X.Preview;
	// A player card still up as the lobby opens (a name caught the player's eye at a table) holds every input under its
	// backdrop, as RiverLine's LobbyPages does: the page, the sheet and the dock; the marquee sits above the backdrop.
	const bool CardUp = Access::CardOpen(RL) && !X.Preview;
	const Pointer Real = X.UI.Ptr;
	const auto HoldForCard = [&X, CardUp]() {
		if (CardUp)
		{
			X.UI.Ptr.Pressed = false;
			X.UI.Ptr.Released = false;
			X.UI.Ptr.Wheel = 0.0f;
			X.UI.Ptr.Active = false;
		}
	};
	HoldForCard();
	// House law 3: an open sheet holds the pointer for everything drawn before it; the dock always holds what's under it.
	rlsites_detail::SitesBlocked Held;
	rlsites_detail::SitesBlocked Pill;
	rlsites_detail::SitesBlocked Raised;
	if (Sheet)
	{
		Held = rlsites_detail::SitesBlockUnder(X.UI, IgniteSheetRect, true);
	}
	else
	{
		Pill = rlsites_detail::SitesBlockUnder(X.UI, {IgniteDockX, IgniteDockY, IgniteDockW, IgniteDockH}, false);
		if (!Pill.Active)
		{
			const float Dx = X.UI.Ptr.X - IgniteRaisedCx;
			const float Dy = X.UI.Ptr.Y - IgniteRaisedCy;
			if (X.UI.Ptr.Active && Dx * Dx + Dy * Dy <= (IgniteRaisedR + 8.0f) * (IgniteRaisedR + 8.0f))
			{
				Raised = rlsites_detail::SitesBlockUnder(X.UI, {744.0f, 836.0f, 112.0f, 112.0f}, false);
			}
		}
	}
	// The page, easing in.
	{
		Canvas& C = X.C;
		const float In = X.Preview ? 1.0f : IgniteEase((X.Now - Access::PageAt(RL)) / 0.4);
		const float A0 = C.GetAlpha();
		C.Save();
		C.SetAlpha(A0 * In);
		C.Translate(0.0f, (1.0f - In) * 16.0f);
		switch (Page)
		{
		case Pg::Lobby: IgnitePlay(X); break;
		case Pg::Series: Access::SeriesPage(RL, X.Now); break;
		default:
			// The other pages (REWARDS, BOARDS, BUZZ, WALLET, the Player Card) draw on the plum ground.
			X.C.FillRect({0.0f, 72.0f, 1600.0f, 890.0f}, IgniteHx(X.K.Plum));
			IgnitePage(X, Page);
			break;
		}
		C.Restore();
		C.SetAlpha(A0);
	}
	rlsites_detail::SitesUnblock(X.UI, Raised);
	rlsites_detail::SitesUnblock(X.UI, Pill);
	if (Sheet)
	{
		rlsites_detail::SitesUnblock(X.UI, Held);
		if (Held.Scrim)
		{
			X.U.Sheet = false; // the scrim's click closes the sheet and is swallowed
			X.S.ConfirmRegister = false;
		}
		else
		{
			IgniteSheet(X);
		}
	}
	if (CardUp)
	{
		X.UI.Ptr = Real;
	}
	IgniteMarquee(X);
	HoldForCard();
	IgniteDock(X, Page == Pg::Lobby && X.U.Chip == 2);
	if (CardUp)
	{
		X.UI.Ptr = Real;
		Access::PlayerCard(RL, X.Now);
	}
}

// ------------------------------------------------------------------ BOARDS: High Scores (SPEC §2.4 I)

void IgniteBoards(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const net::SiteId Site = net::SiteId::Ignite;
	const net::Network& Net = net::Site(Site);
	const std::vector<net::Board> Shown = rlsites_detail::SitesBoards(Site, false);
	net::Board Board = Access::BoardShown(X.RL);
	if (std::find(Shown.begin(), Shown.end(), Board) == Shown.end())
	{
		Board = Shown.front();
		Access::ShowBoard(X.RL, Board, X.Now);
	}
	// The tabs (board<Label>, as RiverLine's): This Month and Season; the Fuse Wall joins them with FUSE (G2).
	float Tx = 24.0f;
	for (const net::Board B : Shown)
	{
		const std::string Label = rlsites_detail::SitesBoardName(Site, B);
		const float W = C.Measure(Label, 15.0f, 800) + 36.0f;
		const Rect R{Tx, 88.0f, W, 36.0f};
		const Ui::ClickState St = X.UI.Clickable("board" + Label, R, !X.Preview);
		const bool On = B == Board;
		if (St.Clicked && !On)
		{
			Access::ShowBoard(X.RL, B, X.Now);
			X.U.BoardAt = X.Now;
		}
		IgnitePill(C, R, On ? Hex(K.Coral) : IgniteHx(St.Hover ? K.Panel : K.Well), IgniteHx(K.Rim), On ? 0.0f : 1.2f);
		C.Text(Label, R.X + W / 2.0f, R.Y + 24.0f, IgniteTx(15.0f, 800, On ? IgniteHx(0x2a0d14) : Hex(K.Cream), Align::Center));
		Tx += W + 10.0f;
	}
	C.Text("HIGH SCORES", Tx + 14.0f, 116.0f, IgniteBi(22.0f, Hex(IgSun)));
	{
		// What the board counts, in two lines at most at the right (a long one wraps rather than running off the page).
		std::vector<std::string> About = rlnet_detail::NetWrap(C, rlsites_detail::SitesBoardAbout(X.S, Site, Board, X.World), 620.0f, 12.5f, 600);
		for (size_t I = 2; I < About.size(); ++I)
		{
			About[1] += " " + About[I]; // the ellipsis ends the second line
		}
		const size_t Lines = std::min<size_t>(About.size(), 2);
		for (size_t I = 0; I < Lines; ++I)
		{
			const float Y = Lines == 1 ? 112.0f : 103.0f + 16.0f * static_cast<float>(I);
			C.Text(About[I], 1576.0f, Y, IgniteTx(12.5f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, 700.0f));
		}
	}
	const net::HeroStats Me = rlsites_detail::SitesHero(X.S, Site, Net, X.World, Board);
	net::BoardRow Mine;
	const std::vector<net::BoardRow> Rows = Net.Leaderboard(Board, X.World, Me, 60, &Mine);
	const auto NameOf = [&](const net::BoardRow& Rw) { return Rw.Player < 0 ? X.S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name; };
	const auto ValueOf = [](net::Board B, double V) {
		const long long N = std::llround(V);
		return B == net::Board::Earnings ? rlnet_detail::NetMoney(static_cast<Chips>(N)) : Grouped(static_cast<int64_t>(N)) + " pts";
	};
	const double Since = X.Preview ? 10.0 : X.Now - std::max(X.U.BoardAt, Access::PageAt(X.RL));
	// The podium: the top three on pedestals, the winner in the middle.
	{
		const Rect P{24.0f, 136.0f, 1552.0f, 220.0f};
		C.FillRoundRect(P, 24.0f, Paint::Linear({0.0f, P.Y}, {0.0f, P.Y + P.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
		C.GlowRoundRect({600.0f, 160.0f, 400.0f, 160.0f}, 80.0f, IgniteHx(IgSun, 0.08f), 60.0f);
		const int Order[3] = {1, 0, 2};
		const float Offs[3] = {-260.0f, 0.0f, 260.0f};
		const float Heights[3] = {78.0f, 58.0f, 44.0f};
		const Color Medals[3] = {Hex(IgSun), Hex(0xd9d4e8), Hex(0xe0905a)};
		for (int I = 0; I < 3; ++I)
		{
			const int Place = Order[I];
			if (Place >= static_cast<int>(Rows.size()))
			{
				continue;
			}
			const net::BoardRow& Rw = Rows[static_cast<size_t>(Place)];
			const float Rise = IgniteEase((Since - 0.12 * (2 - Place)) / 0.6);
			const float Cx = 800.0f + Offs[I];
			const float H = Heights[Place] * Rise;
			C.FillRoundRect({Cx - 110.0f, 348.0f - H, 220.0f, H}, 12.0f, Paint::Linear({0.0f, 348.0f - H}, {0.0f, 348.0f}, IgniteA(Medals[Place], 0.55f), IgniteA(Medals[Place], 0.08f)));
			C.Text(std::to_string(Rw.Rank), Cx, 348.0f - H + 34.0f, IgniteBi(26.0f, Hex(K.Cream), Align::Center));
			const float Ar = Place == 0 ? 34.0f : 28.0f;
			const float Ay = 348.0f - H - Ar - 44.0f;
			if (Place == 0)
			{
				C.GlowRoundRect({Cx - Ar - 6.0f, Ay - Ar - 6.0f, Ar * 2.0f + 12.0f, Ar * 2.0f + 12.0f}, Ar + 6.0f, IgniteA(Medals[0], 0.45f), 18.0f);
				IgniteSticker(C, Cx + Ar * 0.85f, Ay - Ar * 0.75f, 14.0f, "1", Medals[0], IgniteHx(0x2a1640), 13.0f, 12, 0.72f);
			}
			const std::string Nm = NameOf(Rw);
			DrawAvatar(C, Cx, Ay, Ar, AvatarFor(Nm));
			C.StrokeArc(Cx, Ay, Ar + 2.0f, 0.0f, 2.0f * Pi, Medals[Place], 3.0f);
			C.Text(Nm, Cx, Ay + Ar + 22.0f, IgniteTx(16.0f, 800, Rw.Player < 0 ? Hex(K.Coral) : Hex(K.Cream), Align::Center, Baseline::Alphabetic, 230.0f));
			C.Text(ValueOf(Board, Rw.Value), Cx, Ay + Ar + 40.0f, IgniteBi(15.0f, Hex(IgSun), Align::Center));
		}
		if (Rows.empty())
		{
			C.Text(IgniteNoBoard, 800.0f, 252.0f, IgniteTx(16.0f, 600, pal::Muted, Align::Center));
		}
	}
	// The rows from fourth: 40 tall, ranks in BlackItalic, yesterday's arrows, the player's row lit.
	{
		const Rect Area{24.0f, 372.0f, 1552.0f, 388.0f};
		const int Visible = 9;
		const int First = 3;
		for (int I = 0; I < Visible; ++I)
		{
			const size_t Idx = static_cast<size_t>(First + I);
			if (Idx >= Rows.size())
			{
				break;
			}
			const net::BoardRow& Rw = Rows[Idx];
			const float Y = Area.Y + static_cast<float>(I) * 43.0f;
			const float In = IgniteEase((Since - 0.03 * static_cast<double>(I)) / 0.4);
			const float A0 = C.GetAlpha();
			C.SetAlpha(A0 * In);
			const bool Hero = Rw.Player < 0;
			if (Hero)
			{
				C.GlowRoundRect({Area.X, Y, Area.W, 40.0f}, 14.0f, IgniteHx(K.Coral, 0.25f), 10.0f);
			}
			C.FillRoundRect({Area.X, Y, Area.W, 40.0f}, 14.0f, Hero ? IgniteHx(K.Coral, 0.16f) : IgniteHx(I % 2 == 0 ? K.PanelTop : K.PanelBot));
			C.Text(std::to_string(Rw.Rank), Area.X + 44.0f, Y + 29.0f, IgniteBi(22.0f, Hero ? Hex(K.Coral) : Hex(K.Cream), Align::Right));
			if (Rw.Move != 0)
			{
				rlnet_detail::NetTriangle(C, Area.X + 66.0f, Y + 20.0f, 12.0f, Rw.Move > 0, Rw.Move > 0 ? Hex(IgLime) : Hex(IgRed));
				C.Text(std::to_string(std::abs(Rw.Move)), Area.X + 78.0f, Y + 25.0f, IgniteTx(11.0f, 800, Rw.Move > 0 ? Hex(IgLime) : Hex(IgRed)));
			}
			const std::string Nm = NameOf(Rw);
			DrawAvatar(C, Area.X + 124.0f, Y + 20.0f, 14.0f, AvatarFor(Nm));
			C.StrokeArc(Area.X + 124.0f, Y + 20.0f, 15.5f, 0.0f, 2.0f * Pi, Hex(IgViolet), 2.0f);
			C.Text(Nm, Area.X + 150.0f, Y + 26.0f, IgniteTx(15.0f, 700, Hero ? Hex(K.Coral) : Hex(K.Cream), Align::Left, Baseline::Alphabetic, 600.0f));
			C.Text(ValueOf(Board, Rw.Value), Area.X + Area.W - 24.0f, Y + 27.0f, IgniteBi(17.0f, Hex(IgSun), Align::Right));
			C.SetAlpha(A0);
		}
	}
	// The YOU bar, above the dock's raised button (its value's baseline in 790-810: SitesRivalBoardHero's Ignite window).
	{
		const Rect R{24.0f, 768.0f, 1552.0f, 52.0f};
		C.GlowRoundRect(R, 20.0f, IgniteHx(K.Coral, 0.3f), 12.0f);
		C.FillRoundRect(R, 20.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, IgniteHx(K.CoralDeep), IgniteHx(K.Panel)));
		C.Text("YOU", R.X + 22.0f, R.Y + 33.0f, IgniteBi(16.0f, Hex(IgSun)));
		C.Text(Mine.Rank > 0 ? "#" + Grouped(Mine.Rank) : std::string("\xE2\x80\x94"), R.X + 74.0f, R.Y + 35.0f, IgniteBi(22.0f, Hex(K.Cream)));
		C.Text(X.S.HeroName, R.X + 170.0f, R.Y + 33.0f, IgniteTx(16.0f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 420.0f));
		std::string Goal;
		if (Mine.Rank == 0)
		{
			Goal = "A final table here puts you on the board.";
		}
		else if (Mine.Rank > 1)
		{
			const net::BoardRow* Above = nullptr;
			for (const net::BoardRow& Rw : Rows)
			{
				Above = Rw.Rank == Mine.Rank - 1 ? &Rw : Above;
			}
			if (Above)
			{
				Goal = Grouped(std::max<int64_t>(1, static_cast<int64_t>(std::llround(Above->Value - Mine.Value + 1.0)))) + " pts to pass " + Ordinal(Above->Rank);
			}
		}
		else
		{
			Goal = "Top of the board.";
		}
		C.Text(Goal, 1000.0f, R.Y + 33.0f, IgniteTx(14.0f, 700, Hex(IgLime), Align::Right, Baseline::Alphabetic, 400.0f));
		C.Text(Mine.Rank > 0 ? ValueOf(Board, Mine.Value) : std::string("No score"), R.X + R.W - 24.0f, R.Y + 35.0f, IgniteBi(20.0f, Mine.Rank > 0 ? Hex(IgSun) : pal::Muted, Align::Right));
	}
}

// ------------------------------------------------------------------ BUZZ (SPEC §2.4 J)

/** A card's sticker word for a news item (its tag, or its kind's). */
std::string IgniteBuzzTag(const net::NewsItem& It)
{
	if (!It.Tag.empty())
	{
		return It.Tag == "BIG WIN" ? std::string("BIG WIN") : IgniteUpper(It.Tag);
	}
	switch (It.Kind)
	{
	case net::NewsKind::Series: return "SERIES";
	case net::NewsKind::Schedule: return "NEW";
	case net::NewsKind::Record: return "RECORD";
	case net::NewsKind::Hero: return "YOU";
	case net::NewsKind::People: return "PEOPLE";
	default: return "BIG WIN";
	}
}

bool IgnitePromo(const net::NewsItem& It)
{
	return It.Tag == "PROMO" || It.Tag == "LAUNCH" || It.Tag == "OVERLAY" || It.Tag == "NEW LOBBY";
}

void IgniteBuzz(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const char* const Filters[4] = {"Everything", "Big hits", "Promos", "Shouts"};
	float Fx = 24.0f;
	for (int I = 0; I < 4; ++I)
	{
		if (I == 1)
		{
			continue; // Big hits: the FUSEs of x10 and up you sat in (G2); it appears with the first one
		}
		const float W = C.Measure(Filters[I], 14.0f, 800) + 34.0f;
		const Rect R{Fx, 88.0f, W, 34.0f};
		const Ui::ClickState St = X.UI.Clickable(std::string("buzz") + Filters[I], R, !X.Preview);
		if (St.Clicked)
		{
			X.U.Buzz = I;
			X.U.BuzzGoal = 0.0f;
		}
		const bool On = X.U.Buzz == I;
		IgnitePill(C, R, On ? Hex(K.Coral) : IgniteHx(St.Hover ? K.Panel : K.Well), IgniteHx(K.Rim), On ? 0.0f : 1.2f);
		C.Text(Filters[I], R.X + W / 2.0f, R.Y + 22.0f, IgniteTx(14.0f, 800, On ? IgniteHx(0x2a0d14) : Hex(K.Cream), Align::Center));
		Fx += W + 10.0f;
	}
	C.Text("BUZZ", Fx + 18.0f, 116.0f, IgniteBi(26.0f, Hex(IgPink)));
	C.Text("What's lighting up the boardwalk", 1560.0f, 112.0f, IgniteTx(13.0f, 600, pal::Muted, Align::Right));
	// The cards: the site's own news (real items only, no reaction counts), and the members' shouts (flavour, never a result).
	struct BuzzCard
	{
		double At = 0.0;
		bool Shout = false;
		std::string Tag, Title, Body, Who;
		Chips Amount = 0;
	};
	std::vector<BuzzCard> Cards;
	if (X.U.Buzz != 3)
	{
		for (const net::NewsItem& It : Access::NewsFeed(X.RL))
		{
			if (X.U.Buzz == 2 && !IgnitePromo(It))
			{
				continue;
			}
			BuzzCard Bc;
			Bc.At = It.At;
			Bc.Tag = IgniteBuzzTag(It);
			Bc.Title = It.Title;
			Bc.Body = It.Body;
			Bc.Amount = It.Amount;
			Cards.push_back(Bc);
		}
	}
	if (X.U.Buzz == 0 || X.U.Buzz == 3)
	{
		// One shout every 10 world minutes; the last two hours of them.
		const net::Network& Net = net::Site(net::SiteId::Ignite);
		const std::vector<net::Player>& Members = Net.Players();
		const std::vector<std::string>& Lines = IgniteShouts();
		const double Slot = std::floor(X.World / 10.0);
		// {Event} and {Time} are tonight's featured event only: a line that says "tonight" never names tomorrow's.
		std::vector<net::EventInstance> Tonight = IgniteFeaturedNow(X, 1);
		if (!Tonight.empty() && !IgniteTonight(Tonight.front().Start, X.World))
		{
			Tonight.clear();
		}
		bool Cashed100 = false;
		for (const HistoryEntry& H : X.S.History)
		{
			Cashed100 = Cashed100 || (net::SiteOf(H.EventId) == net::SiteId::Ignite && H.Prize >= 10000);
		}
		// Everything mixes the last 40 minutes' shouts in with the news; Shouts shows the last two hours'.
		const int Shouts = X.U.Buzz == 3 ? 12 : 4;
		for (int Back = 0; Back < Shouts; ++Back)
		{
			const double At = (Slot - static_cast<double>(Back)) * 10.0;
			const int Line = IgniteShoutAt(X.S.WorldSeed(), At);
			if (Line < 0 || Line >= static_cast<int>(Lines.size()))
			{
				continue;
			}
			std::string Text = Lines[static_cast<size_t>(Line)];
			// The slots: tonight's event and its time; your handle only after your first Ignite cash of $100 or more; a
			// Headliner's name only once Headliners headline (G2). A line that says what time it is only at its hours, as of
			// when it was shouted (G1_AUDIT L6: "Evening, boardwalk!" never at 9 AM).
			if (Text.find("{Headliner}") != std::string::npos || (Text.find("{Hero}") != std::string::npos && !Cashed100) ||
				((Text.find("{Event}") != std::string::npos || Text.find("{Time}") != std::string::npos) && Tonight.empty()) || !IgniteShoutFits(Text, At))
			{
				continue;
			}
			if (!Tonight.empty())
			{
				Text = rlsites_detail::SitesFill(Text, {{"Event", Net.TemplateOf(Tonight.front()).Name}, {"Time", rlnet_detail::LiveTime(Tonight.front().Start)}});
			}
			Text = rlsites_detail::SitesFill(Text, {{"Hero", X.S.HeroName}});
			BuzzCard Bc;
			Bc.At = At;
			Bc.Shout = true;
			Bc.Tag = "SHOUT";
			Bc.Body = Text;
			if (!Members.empty())
			{
				const uint32_t Pick = IgniteMix(Fnv1a(std::to_string(X.S.WorldSeed()) + ":ignite:shouter:" + std::to_string(static_cast<long long>(At))));
				Bc.Who = Members[Pick % Members.size()].Name;
			}
			Cards.push_back(Bc);
		}
	}
	std::stable_sort(Cards.begin(), Cards.end(), [](const BuzzCard& A, const BuzzCard& B) { return A.At > B.At; });
	// The masonry: three columns 500 wide, each card as tall as its words (120-260), the wheel scrolling.
	const Rect Area{24.0f, 136.0f, 1552.0f, 696.0f};
	if (X.UI.Hover(Area) && X.UI.Ptr.Wheel != 0.0f)
	{
		X.U.BuzzGoal += X.UI.Ptr.Wheel > 0.0f ? 160.0f : -160.0f;
	}
	X.U.BuzzGoal = std::max(0.0f, X.U.BuzzGoal);
	X.U.BuzzScroll += (X.U.BuzzGoal - X.U.BuzzScroll) * IgniteFollow(X.Dt, 12.0);
	if (X.Preview || std::fabs(X.U.BuzzGoal - X.U.BuzzScroll) < 0.4f)
	{
		X.U.BuzzScroll = X.U.BuzzGoal;
	}
	if (Cards.empty())
	{
		C.Text(X.U.Buzz == 3 ? IgniteNoShouts : IgniteNoNews, 800.0f, 420.0f, IgniteTx(17.0f, 600, pal::Muted, Align::Center));
		return;
	}
	float Col[3] = {Area.Y, Area.Y, Area.Y};
	C.PushClip(Area);
	// The shouts' tints colour a name on the card and its sticker's ground: the lighter violet (Electric Violet reads 3.8:1).
	const uint32_t Tints[5] = {IgPink, K.Sky, IgSun, IgLime, IgNightAccent};
	for (size_t I = 0; I < Cards.size() && I < 40; ++I)
	{
		const BuzzCard& Bc = Cards[I];
		const int Lane = static_cast<int>(std::min_element(Col, Col + 3) - Col);
		const float Cx = Area.X + static_cast<float>(Lane) * 526.0f;
		const std::vector<std::string> Body = rlnet_detail::NetWrap(C, Bc.Body, 380.0f, 14.0f, 500);
		const float H = std::min(260.0f, std::max(120.0f, (Bc.Shout ? 78.0f : 96.0f) + static_cast<float>(std::min<size_t>(Body.size(), 5)) * 20.0f));
		const float Y = Col[Lane] - X.U.BuzzScroll;
		Col[Lane] += H + 22.0f;
		if (Y > Area.Y + Area.H || Y + H < Area.Y)
		{
			continue;
		}
		const Color Tint = Bc.Shout ? Hex(Tints[I % 5]) : Bc.Tag == "YOU" ? Hex(IgSun) : Bc.Tag == "OVERLAY" ? Hex(IgLime) : Bc.Tag == "PROMO" || Bc.Tag == "LAUNCH" ? Hex(K.Coral) : Hex(IgPink);
		const Rect R{Cx, Y, 500.0f, H};
		C.FillRoundRect({R.X, R.Y + 4.0f, R.W, R.H}, 22.0f, Rgba(0, 0, 0, 0.3f));
		C.FillRoundRect(R, 22.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
		C.StrokeRoundRect(R, 22.0f, IgniteA(Tint, 0.45f), 1.2f);
		C.FillRoundRect({R.X, R.Y, 6.0f, R.H}, 3.0f, IgniteA(Tint, 0.8f));
		const float Sw = std::max(26.0f, C.Measure(Bc.Tag, 10.0f, 900) * 0.62f + 14.0f);
		IgniteSticker(C, R.X + R.W - Sw - 12.0f, R.Y + 26.0f, Sw, Bc.Tag, Tint, IgniteHx(0x1a0f2a), 10.0f, 16, 0.84f);
		float Ty = R.Y + 34.0f;
		if (Bc.Shout)
		{
			DrawAvatar(C, R.X + 34.0f, R.Y + 34.0f, 14.0f, AvatarFor(Bc.Who));
			C.Text(Bc.Who, R.X + 56.0f, R.Y + 39.0f, IgniteTx(14.0f, 800, Tint, Align::Left, Baseline::Alphabetic, 300.0f));
			Ty = R.Y + 70.0f;
		}
		else
		{
			Ty = rlnet_detail::NetParagraph(C, Bc.Title, R.X + 24.0f, R.Y + 36.0f, R.W - 2.0f * Sw - 60.0f, 17.0f, 800, Hex(K.Cream), 22.0f, 2) + 4.0f;
		}
		for (size_t L = 0; L < Body.size() && L < 5; ++L)
		{
			C.Text(Body[L], R.X + 24.0f, Ty + static_cast<float>(L) * 20.0f, IgniteTx(14.0f, 500, IgniteHx(K.Cream, 0.82f)));
		}
		C.Text(rlnet_detail::NetAgo(X.World - Bc.At, Bc.At), R.X + 24.0f, R.Y + R.H - 16.0f, IgniteTx(11.5f, 700, pal::Muted));
		if (Bc.Amount > 0)
		{
			C.Text(Money(Bc.Amount), R.X + R.W - 22.0f, R.Y + R.H - 14.0f, IgniteBi(20.0f, Hex(IgSun), Align::Right));
		}
	}
	C.PopClip();
	const float Bottom = std::max(Col[0], std::max(Col[1], Col[2]));
	X.U.BuzzGoal = std::min(X.U.BuzzGoal, std::max(0.0f, Bottom - Area.Y - Area.H + 20.0f));
}

// ------------------------------------------------------------------ WALLET (SPEC §2.4 K)

/** "Sat to Mon" (or one day's name and time when the window falls inside a day): when money leaving arrives. */
std::string IgniteWindow(double From, double HoursMin, double HoursMax)
{
	const int A = net::DayOf(From + HoursMin * 60.0);
	const int B = net::DayOf(From + HoursMax * 60.0);
	if (A == B)
	{
		return std::string(net::WeekdayName(A)) + " " + net::TimeLabel(From + HoursMin * 60.0);
	}
	return std::string(net::WeekdayName(A)) + " to " + net::WeekdayName(B);
}

const Chips IgniteDepositSteps[4] = {1000, 5000, 10000, 50000};
const Chips IgniteWithdrawSteps[4] = {2000, 5000, 10000, 20000};

void IgniteWallet(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const net::SiteId Site = net::SiteId::Ignite;
	const site::Account* A = IgniteAccount(X.S);
	rlsites_detail::SitesUi& Su = rlsites_detail::SitesOf(&X.RL);
	const net::CashMethod* In = site::MethodFor(Site, std::string(), false);
	const net::CashMethod* Out = site::MethodFor(Site, std::string(), true);
	const Chips Balance = A ? A->Cents : 0;
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesLook(Site);
	const float Ease = X.Preview ? 1.0f : IgniteEase((X.Now - Access::PageAt(X.RL)) / 0.6);
	const auto Answer = [&](const std::string& Why, const std::string& Done) {
		Su.Note = Why.empty() ? Done : Why;
		Su.NoteBad = !Why.empty();
		Su.NoteAt = X.Now;
	};
	const auto Card = [&](const Rect& R) {
		C.FillRoundRect({R.X, R.Y + 5.0f, R.W, R.H}, 24.0f, Rgba(0, 0, 0, 0.3f));
		C.FillRoundRect(R, 24.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
		C.StrokeRoundRect(R, 24.0f, IgniteHx(K.Rim), 1.0f);
	};
	// The head: the balance, what's on its way back, the welcome bonus's Fuel Gauge.
	{
		const Rect H{24.0f, 88.0f, 1552.0f, 180.0f};
		Card(H);
		C.FillRoundRect(H, 24.0f, Paint::Linear({H.X, 0.0f}, {H.X + H.W, 0.0f}, IgniteHx(K.Coral, 0.14f), IgniteHx(IgViolet, 0.04f)));
		C.GlowRoundRect({H.X + 30.0f, H.Y + 40.0f, 100.0f, 100.0f}, 50.0f, IgniteHx(IgSun, 0.25f), 24.0f);
		C.FillCircle(H.X + 80.0f, H.Y + 90.0f, 46.0f, Paint::Radial({H.X + 66.0f, H.Y + 74.0f}, 0.0f, {H.X + 80.0f, H.Y + 90.0f}, 46.0f, Hex(0xffe08a), 0.55f, Hex(IgSun), Hex(IgTang)));
		C.StrokeArc(H.X + 80.0f, H.Y + 90.0f, 36.0f, 0.0f, 2.0f * Pi, IgniteHx(0x7a4a00, 0.6f), 3.0f);
		IgniteFlame(C, H.X + 80.0f, H.Y + 112.0f, 30.0f, IgniteHx(0x7a4a00, 0.85f), IgniteHx(0xb06a10, 0.85f), IgniteHx(0xffe08a));
		C.Text("Your balance on Ignite", H.X + 150.0f, H.Y + 44.0f, IgniteTx(16.0f, 700, pal::Muted));
		C.Text(Money(static_cast<Chips>(static_cast<double>(Balance) * static_cast<double>(Ease))), H.X + 150.0f, H.Y + 112.0f, IgniteBi(64.0f, Hex(IgSun)));
		C.Text(IgniteStaysHere, H.X + 150.0f, H.Y + 146.0f, IgniteTx(14.0f, 600, pal::Muted));
		const Chips Coming = rlsites_detail::SitesInTransit(X.S, Site);
		C.Text("ON ITS WAY BACK", H.X + 800.0f, H.Y + 44.0f, IgniteTx(11.0f, 800, pal::Muted));
		C.Text(Coming > 0 ? Money(Coming) : std::string("\xE2\x80\x94"), H.X + 800.0f, H.Y + 90.0f, IgniteBi(30.0f, Coming > 0 ? Hex(K.Cream) : pal::Dim));
		C.Text(Coming > 0 ? std::string("to your bank") : std::string(IgniteNothingComing), H.X + 800.0f, H.Y + 114.0f, IgniteTx(13.0f, 600, pal::Muted));
		C.Text("Bank " + Money(X.S.BankrollCents), H.X + 800.0f, H.Y + 146.0f, IgniteTx(14.0f, 700, Hex(K.Cream)));
		const IgniteWelcome W = IgniteWelcomeOf(X.S, X.World);
		IgniteFuelGauge(C, H.X + 1180.0f, H.Y + 128.0f, 66.0f, W.Started ? W.Needle : 0.0, K);
		C.Text("FUEL GAUGE", H.X + 1180.0f, H.Y + 152.0f, IgniteTx(10.0f, 900, pal::Muted, Align::Center));
		C.Text("WELCOME BONUS", H.X + 1270.0f, H.Y + 52.0f, IgniteBi(15.0f, Hex(K.Cream)));
		std::string L1;
		std::string L2;
		if (!W.Started)
		{
			L1 = "100% up to $100";
			L2 = "your first deposit starts it";
		}
		else
		{
			L1 = Money(W.Released) + " released";
			L2 = W.Closed ? std::string("the 60 days are up") : W.Done ? std::string("all of it's yours") : Money(W.Pending) + " still pending";
		}
		C.Text(L1, H.X + 1270.0f, H.Y + 80.0f, IgniteTx(15.0f, 800, Hex(IgLime), Align::Left, Baseline::Alphabetic, 270.0f));
		C.Text(L2, H.X + 1270.0f, H.Y + 102.0f, IgniteTx(13.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 270.0f));
		if (W.Started && !W.Closed && !W.Done && W.NextIn > 0)
		{
			C.Text("next $5 in " + Money(W.NextIn) + " of rake", H.X + 1270.0f, H.Y + 124.0f, IgniteTx(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, 270.0f));
		}
	}
	// Deposit: the steps (cashin0-3, their baselines at 417: SitesCashier's 380-440 window), what the bank pays, the button.
	{
		const Rect D{24.0f, 296.0f, 500.0f, 436.0f};
		Card(D);
		C.Text("DEPOSIT", D.X + 26.0f, D.Y + 46.0f, IgniteBi(26.0f, Hex(K.Cream)));
		C.Text(IgniteDepositHow, D.X + 26.0f, D.Y + 72.0f, IgniteTx(13.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, D.W - 52.0f));
		Su.Deposit = std::min(std::max(Su.Deposit, 0), 3);
		const float Cw = (D.W - 52.0f - 30.0f) / 4.0f;
		for (int I = 0; I < 4; ++I)
		{
			const Rect Ch{D.X + 26.0f + static_cast<float>(I) * (Cw + 10.0f), 390.0f, Cw, 42.0f};
			const Ui::ClickState St = X.UI.Clickable("cashin" + std::to_string(I), Ch, !X.Preview);
			if (St.Clicked)
			{
				Su.Deposit = I;
			}
			const bool On = Su.Deposit == I;
			IgnitePill(C, Ch, On ? IgniteHx(K.Coral, 0.25f) : IgniteHx(St.Hover ? K.Panel : K.Well), On ? Hex(K.Coral) : IgniteHx(K.Rim), On ? 1.5f : 1.0f);
			C.Text(rlnet_detail::NetMoney(IgniteDepositSteps[I]), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, IgniteBi(17.0f, On ? Hex(K.Cream) : pal::Muted, Align::Center));
		}
		const Chips Amount = IgniteDepositSteps[Su.Deposit];
		const Chips Fee = In ? static_cast<Chips>(std::llround(static_cast<double>(Amount) * static_cast<double>(In->FeePermille) / 1000.0)) : 0;
		const std::pair<std::string, std::string> Rows[4] = {
			{"Lands on Ignite", Money(Amount)},
			{In ? In->Name + " fee (" + Fixed(static_cast<double>(In->FeePermille) / 10.0, In->FeePermille % 10 == 0 ? 0 : 1) + "%)" : std::string("Fee"), Money(Fee)},
			{"The bank pays", Money(Amount + Fee)},
			{"Bank after", Money(X.S.BankrollCents - Amount - Fee)},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float Ry = D.Y + 176.0f + static_cast<float>(I) * 32.0f;
			C.Text(Rows[I].first, D.X + 26.0f, Ry, IgniteTx(14.5f, 600, I == 2 ? Hex(K.Cream) : pal::Muted));
			C.Text(Rows[I].second, D.X + D.W - 26.0f, Ry, IgniteTx(15.0f, 800, I == 0 ? Hex(IgSun) : I == 3 && X.S.BankrollCents - Amount - Fee < 0 ? pal::Red : Hex(K.Cream), Align::Right));
		}
		std::string Why;
		if (!In)
		{
			Why = "No way in yet";
		}
		else if (In->MinCents > 0 && Amount < In->MinCents)
		{
			Why = "Minimum " + rlnet_detail::NetMoney(In->MinCents);
		}
		else if (X.S.BankrollCents < Amount + Fee)
		{
			Why = "Short " + Money(Amount + Fee - X.S.BankrollCents) + " in the bank";
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Primary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !X.Preview;
		O.Sub = Why.empty() ? (In ? In->Name + " \xC2\xB7 lands at once" : std::string()) : Why;
		if (rlsites_detail::SitesButton(X.UI, C, "cashdeposit", {D.X + 26.0f, D.Y + 314.0f, D.W - 52.0f, 64.0f}, "Deposit " + rlnet_detail::NetMoney(Amount), O, Look) && In)
		{
			Answer(X.S.Deposit(Site, Amount, In->Id), "Deposited " + Money(Amount) + ". It's on Ignite now.");
		}
		rlnet_detail::NetParagraph(C, IgniteOnlyBank, D.X + 26.0f, D.Y + 408.0f, D.W - 52.0f, 12.5f, 600, pal::Muted, 17.0f, 2); // Dim reads under 4.5:1 here
	}
	// Withdraw: the steps (cashout0-4, the same row), what reaches the bank and when, the button.
	{
		const Rect Wd{540.0f, 296.0f, 500.0f, 436.0f};
		Card(Wd);
		C.Text("WITHDRAW", Wd.X + 26.0f, Wd.Y + 46.0f, IgniteBi(26.0f, Hex(K.Cream)));
		C.Text(IgniteWithdrawHow, Wd.X + 26.0f, Wd.Y + 72.0f, IgniteTx(13.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, Wd.W - 52.0f));
		if (Su.Withdraw <= 0)
		{
			Su.Withdraw = IgniteWithdrawSteps[0];
		}
		const float Cw = (Wd.W - 52.0f - 40.0f) / 5.0f;
		for (int I = 0; I < 5; ++I)
		{
			const Rect Ch{Wd.X + 26.0f + static_cast<float>(I) * (Cw + 10.0f), 390.0f, Cw, 42.0f};
			const Chips Step = I < 4 ? IgniteWithdrawSteps[I] : Balance;
			const Ui::ClickState St = X.UI.Clickable("cashout" + std::to_string(I), Ch, !X.Preview);
			if (St.Clicked)
			{
				Su.Withdraw = Step;
			}
			const bool On = Su.Withdraw == Step && (I < 4 || Balance > 0);
			IgnitePill(C, Ch, On ? IgniteHx(K.Coral, 0.25f) : IgniteHx(St.Hover ? K.Panel : K.Well), On ? Hex(K.Coral) : IgniteHx(K.Rim), On ? 1.5f : 1.0f);
			C.Text(I < 4 ? rlnet_detail::NetMoney(Step) : std::string("All"), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, IgniteBi(16.0f, On ? Hex(K.Cream) : pal::Muted, Align::Center));
		}
		const Chips Take = Su.Withdraw;
		const Chips OutFee = Out ? Out->WithdrawFeeCents : 0;
		const std::string When = Out ? IgniteWindow(X.World, Out->WithdrawHoursMin, Out->WithdrawHoursMax) : std::string("\xE2\x80\x94");
		const std::pair<std::string, std::string> Rows[4] = {
			{"Leaves Ignite now", Money(Take)},
			{Out ? Out->Name + " fee" : std::string("Fee"), Money(OutFee)},
			{"Reaches your bank", Money(std::max<Chips>(0, Take - OutFee))},
			{"Expected", When},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float Ry = Wd.Y + 176.0f + static_cast<float>(I) * 32.0f;
			C.Text(Rows[I].first, Wd.X + 26.0f, Ry, IgniteTx(14.5f, 600, I == 2 ? Hex(K.Cream) : pal::Muted));
			C.Text(Rows[I].second, Wd.X + Wd.W - 26.0f, Ry, IgniteTx(15.0f, 800, I == 2 ? Hex(IgSun) : Hex(K.Cream), Align::Right));
		}
		std::string Why;
		const Chips Min = net::Def(Site).MinWithdraw;
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
			Why = "Minimum " + rlnet_detail::NetMoney(Min);
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !X.Preview;
		O.Sub = Why.empty() ? "Expected " + When : Why;
		if (rlsites_detail::SitesButton(X.UI, C, "cashwithdraw", {Wd.X + 26.0f, Wd.Y + 314.0f, Wd.W - 52.0f, 64.0f}, "Withdraw " + Money(Take), O, Look) && Out)
		{
			Answer(X.S.Withdraw(Site, Take, Out->Id), "Requested. " + Money(std::max<Chips>(0, Take - OutFee)) + " is on its way to your bank.");
			Su.Withdraw = IgniteWithdrawSteps[0];
		}
		if (!Su.Note.empty() && X.Now - Su.NoteAt < 6.0)
		{
			const float A0 = C.GetAlpha();
			C.SetAlpha(A0 * static_cast<float>(std::min(1.0, (6.0 - (X.Now - Su.NoteAt)) / 0.6)));
			rlnet_detail::NetParagraph(C, Su.Note, Wd.X + 26.0f, Wd.Y + 408.0f, Wd.W - 52.0f, 13.0f, 700, Su.NoteBad ? pal::Red : Hex(IgLime), 17.0f, 2);
			C.SetAlpha(A0);
		}
	}
	// Receipts: the site's own lines, newest first.
	{
		const Rect Rt{1056.0f, 296.0f, 520.0f, 536.0f};
		Card(Rt);
		C.Text("RECEIPTS", Rt.X + 26.0f, Rt.Y + 42.0f, IgniteBi(22.0f, Hex(K.Cream)));
		if (!A || A->Lines.empty())
		{
			rlnet_detail::NetParagraph(C, IgniteNoLines, Rt.X + 26.0f, Rt.Y + 80.0f, Rt.W - 52.0f, 14.0f, 500, pal::Muted, 20.0f, 3);
		}
		else
		{
			float Ly = Rt.Y + 62.0f;
			int Row = 0;
			for (auto It = A->Lines.rbegin(); It != A->Lines.rend() && Ly + 48.0f <= Rt.Y + Rt.H - 10.0f; ++It, ++Row)
			{
				const site::Line& L = *It;
				C.Text(L.Label, Rt.X + 26.0f, Ly + 20.0f, IgniteTx(14.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, Rt.W - 180.0f));
				C.Text(std::string(net::WeekdayName(net::DayOf(L.At))) + " " + net::TimeLabel(L.At), Rt.X + 26.0f, Ly + 38.0f, IgniteTx(12.0f, 500, pal::Muted));
				C.Text(L.Cents == 0 ? std::string("\xE2\x80\x94") : (L.Cents > 0 ? "+" : "\xE2\x88\x92") + Money(L.Cents > 0 ? L.Cents : -L.Cents), Rt.X + Rt.W - 26.0f, Ly + 28.0f,
					IgniteBi(16.0f, L.Cents > 0 ? Hex(IgLime) : L.Cents < 0 ? Hex(K.Cream) : pal::Muted, Align::Right));
				C.FillRect({Rt.X + 26.0f, Ly + 48.0f, Rt.W - 52.0f, 1.0f}, IgniteHx(K.Rim, 0.6f));
				Ly += 52.0f;
			}
		}
	}
	// On the way: each withdrawal as a fuse burning toward the bank (its progress the elapsed share of the wait).
	{
		const Rect Ow{24.0f, 748.0f, 1016.0f, 84.0f};
		Card(Ow);
		C.Text("ON THE WAY", Ow.X + 26.0f, Ow.Y + 30.0f, IgniteBi(16.0f, Hex(K.Cream)));
		int Shown = 0;
		for (const site::Transfer& T : X.S.Life.Sites.Transfers)
		{
			if (T.Site != Site || !T.Out || T.State != 0 || Shown >= 1)
			{
				continue;
			}
			const net::CashMethod* M = Out;
			const bool Range = M && M->WithdrawHoursMax > M->WithdrawHoursMin;
			const std::string Due = Range ? IgniteWindow(T.RequestedAt, M->WithdrawHoursMin, M->WithdrawHoursMax) : std::string(net::WeekdayName(net::DayOf(T.ArrivesAt))) + " " + net::TimeLabel(T.ArrivesAt);
			C.Text("Withdrawal of " + Money(T.Cents) + " \xC2\xB7 requested " + net::WeekdayName(net::DayOf(T.RequestedAt)) + " " + net::TimeLabel(T.RequestedAt), Ow.X + 150.0f, Ow.Y + 30.0f,
				IgniteTx(14.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 460.0f));
			C.Text(Money(std::max<Chips>(0, T.Cents - T.FeeCents)) + " arriving " + Due, Ow.X + Ow.W - 26.0f, Ow.Y + 30.0f, IgniteTx(14.0f, 800, Hex(K.Sky), Align::Right));
			const double Share = T.ArrivesAt > T.RequestedAt ? (X.World - T.RequestedAt) / (T.ArrivesAt - T.RequestedAt) : 1.0;
			IgniteFuseRope(C, Ow.X + 26.0f, Ow.X + Ow.W - 26.0f, Ow.Y + 58.0f, 3.0f, Share, Hex(0xc9a46a), IgniteHx(0x5a4060), X.Tm, true);
			++Shown;
		}
		if (Shown == 0)
		{
			C.Text(IgniteNothingComing, Ow.X + 150.0f, Ow.Y + 30.0f, IgniteTx(14.0f, 600, pal::Muted));
		}
	}
}

// ------------------------------------------------------------------ the Player Card (SPEC §2.4 L)

void IgniteCard(IgniteCtx& X)
{
	Canvas& C = X.C;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	const site::Account* A = IgniteAccount(X.S);
	// The holographic card: a shimmer sweep across it, the bubble, the name, how long you've been here.
	{
		const Rect R{24.0f, 88.0f, 520.0f, 744.0f};
		C.GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 28.0f, Rgba(0, 0, 0, 0.45f), 24.0f);
		C.FillRoundRect(R, 28.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, IgniteHx(0x3a1f5c), IgniteHx(0x1a0f2a)));
		C.FillRoundRect(R, 28.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, IgniteHx(K.Coral, 0.18f), IgniteHx(K.Sky, 0.12f)));
		const float Sweep = static_cast<float>(std::fmod(X.Tm / 4.0, 1.0));
		C.PushClip(R);
		const float Sx = R.X - 200.0f + Sweep * (R.W + 400.0f);
		C.FillPolygon({{Sx, R.Y}, {Sx + 90.0f, R.Y}, {Sx - 110.0f, R.Y + R.H}, {Sx - 200.0f, R.Y + R.H}}, Rgba(255, 255, 255, 0.07f));
		C.PopClip();
		IgniteNeonRoundRect(C, R, 28.0f, IgniteHx(K.Coral, 0.7f), 2.0f, 6.0f);
		C.GlowRoundRect({R.X + 170.0f, R.Y + 50.0f, 180.0f, 180.0f}, 90.0f, IgniteHx(IgViolet, 0.35f), 30.0f);
		DrawAvatar(C, R.X + 260.0f, R.Y + 150.0f, 86.0f, AvatarFor(X.S.HeroName));
		C.StrokeArc(R.X + 260.0f, R.Y + 150.0f, 92.0f, 0.0f, 2.0f * Pi, Hex(IgViolet), 5.0f);
		C.Text(X.S.HeroName, R.X + 260.0f, R.Y + 290.0f, IgniteBi(34.0f, Hex(K.Cream), Align::Center, Baseline::Alphabetic, R.W - 40.0f));
		if (A)
		{
			C.Text("On Ignite since " + net::DateLabel(net::DayOf(A->JoinedAt)), R.X + 260.0f, R.Y + 320.0f, IgniteTx(15.0f, 700, Hex(K.Coral), Align::Center));
		}
		C.FillRect({R.X + 40.0f, R.Y + 350.0f, R.W - 80.0f, 1.0f}, IgniteHx(K.Cream, 0.15f));
		const std::pair<std::string, std::string> Facts[4] = {
			{A ? Grouped(A->LifetimeEvents) : std::string("0"), "EVENTS HERE"},
			{A ? rlnet_detail::NetMoney(A->LifetimeWon) : std::string("$0"), "WON HERE"},
			{A ? Grouped(static_cast<int64_t>(std::llround(A->Hero.Points))) : std::string("0"), "SEASON POINTS"},
			{A ? Grouped(A->Hero.FinalTables) : std::string("0"), "FINAL TABLES"},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float Fx = R.X + 60.0f + static_cast<float>(I % 2) * 220.0f;
			const float Fy = R.Y + 420.0f + static_cast<float>(I / 2) * 96.0f;
			C.Text(Facts[I].first, Fx, Fy, IgniteBi(34.0f, I == 1 ? Hex(IgSun) : Hex(K.Cream)));
			C.Text(Facts[I].second, Fx, Fy + 24.0f, IgniteTx(10.5f, 900, pal::Muted));
		}
		siteart::Logo(C, {R.X + 160.0f, R.Y + R.H - 70.0f, 200.0f, 40.0f}, net::SiteId::Ignite);
	}
	// The tabs: at G1, RECORDS (the Locker, the Sticker Book and Rivals open with the loops, G2).
	{
		const Rect Tb{560.0f, 88.0f, 160.0f, 40.0f};
		IgnitePill(C, Tb, Hex(K.Coral), Color(), 0.0f);
		C.Text("RECORDS", Tb.X + Tb.W / 2.0f, Tb.Y + 26.0f, IgniteTx(15.0f, 900, IgniteHx(0x2a0d14), Align::Center));
	}
	// RECORDS: from the account and History (best finish, biggest cash, titles, the last results here).
	{
		const Rect P{560.0f, 140.0f, 1016.0f, 692.0f};
		C.FillRoundRect(P, 24.0f, Paint::Linear({0.0f, P.Y}, {0.0f, P.Y + P.H}, IgniteHx(K.PanelTop), IgniteHx(K.PanelBot)));
		C.StrokeRoundRect(P, 24.0f, IgniteHx(K.Rim), 1.0f);
		std::vector<const HistoryEntry*> Here;
		for (const HistoryEntry& H : X.S.History)
		{
			if (net::SiteOf(H.EventId) == net::SiteId::Ignite)
			{
				Here.push_back(&H);
			}
		}
		const HistoryEntry* Best = nullptr;
		const HistoryEntry* Biggest = nullptr;
		int Titles = 0;
		int Cashes = 0;
		for (const HistoryEntry* H : Here)
		{
			Best = !Best || H->Place < Best->Place || (H->Place == Best->Place && H->Entrants > Best->Entrants) ? H : Best;
			Biggest = H->Prize > 0 && (!Biggest || H->Prize > Biggest->Prize) ? H : Biggest;
			Titles += H->Place == 1 ? 1 : 0;
			Cashes += H->Prize > 0 ? 1 : 0;
		}
		const std::pair<std::string, std::string> Big[4] = {
			{Best ? Ordinal(Best->Place) + " of " + Grouped(Best->Entrants) : std::string("\xE2\x80\x94"), "BEST FINISH"},
			{Biggest ? Money(Biggest->Prize) : std::string("\xE2\x80\x94"), "BIGGEST CASH"},
			{std::to_string(Titles), "TITLES"},
			{std::to_string(Cashes), "CASHES"},
		};
		for (int I = 0; I < 4; ++I)
		{
			const Rect Tile{P.X + 24.0f + static_cast<float>(I) * 244.0f, P.Y + 24.0f, 228.0f, 110.0f};
			C.FillRoundRect(Tile, 18.0f, IgniteHx(K.Well));
			C.Text(Big[I].first, Tile.X + 18.0f, Tile.Y + 56.0f, IgniteBi(28.0f, I == 1 ? Hex(IgSun) : Hex(K.Cream), Align::Left, Baseline::Alphabetic, Tile.W - 30.0f));
			C.Text(Big[I].second, Tile.X + 18.0f, Tile.Y + 84.0f, IgniteTx(10.5f, 900, pal::Muted));
		}
		if (Biggest)
		{
			C.Text(Biggest->Name, P.X + 268.0f + 18.0f, P.Y + 126.0f, IgniteTx(11.0f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 200.0f));
		}
		C.Text("YOUR LAST RESULTS HERE", P.X + 24.0f, P.Y + 176.0f, IgniteBi(16.0f, Hex(K.Cream)));
		if (Here.empty())
		{
			C.Text(IgniteNoRecords, P.X + 24.0f, P.Y + 214.0f, IgniteTx(15.0f, 600, pal::Muted));
		}
		for (size_t I = 0; I < Here.size() && I < 10; ++I)
		{
			const HistoryEntry& H = *Here[I];
			const float Y = P.Y + 196.0f + static_cast<float>(I) * 46.0f;
			C.FillRoundRect({P.X + 24.0f, Y, P.W - 48.0f, 40.0f}, 14.0f, IgniteHx(I % 2 == 0 ? K.Well : K.PanelBot));
			C.Text(H.Name, P.X + 44.0f, Y + 26.0f, IgniteTx(15.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 420.0f));
			C.Text(Ordinal(H.Place) + " of " + Grouped(H.Entrants), P.X + 560.0f, Y + 26.0f, IgniteTx(14.0f, 700, H.Place <= 3 ? Hex(IgSun) : pal::Muted));
			C.Text(net::DateLabel(net::DayOf(net::EntryStart(H))), P.X + 740.0f, Y + 26.0f, IgniteTx(13.0f, 600, pal::Muted));
			C.Text(H.Prize > 0 ? "+" + Money(H.Prize) : std::string("\xE2\x80\x94"), P.X + P.W - 44.0f, Y + 27.0f, IgniteBi(17.0f, H.Prize > 0 ? Hex(IgLime) : pal::Dim, Align::Right));
		}
	}
}

// ------------------------------------------------------------------ "Your run" (SPEC §2.4 G)

/** The next start of an event's family (the same template, seats not yet closed), for Play again. */
bool IgniteAgain(const std::string& EventId, double World, net::EventInstance& Out)
{
	// The instance id is "<template>@<start minute>": its family is the template (and the launch week's one-offs of it).
	const net::Network& Net = net::ForEvent(EventId);
	std::string Template = EventId.substr(0, EventId.find('@'));
	const size_t Launch = Template.find("-launch");
	Template = Launch == std::string::npos ? Template : Template.substr(0, Launch);
	for (const net::EventInstance& E : Net.Window(World - 60.0, World + 7.0 * net::MinutesPerDay))
	{
		const std::string& Id = Net.TemplateOf(E).Id;
		if (E.Id == EventId || (Id != Template && Id.rfind(Template + "-launch", 0) != 0))
		{
			continue;
		}
		const net::Status St = Net.Live(E, World).St;
		if (St == net::Status::Announced || St == net::Status::Registering || St == net::Status::LateReg)
		{
			Out = E;
			return true;
		}
	}
	return false;
}
} // namespace rlignite_detail

namespace riverline_detail
{
// RiverLine.cpp's (the shared results screen's): whose money a result was (a package's roll, buyers' pieces), "" when it
// was all the player's.
std::string ResultSplit(const Session& S, const Results& R);
} // namespace riverline_detail

namespace rlignite_detail
{
void IgniteResults(IgniteCtx& X)
{
	Canvas& C = X.C;
	Ui& UI = X.UI;
	const igniteart::IgniteTones& K = X.K;
	using namespace igniteart;
	IgniteMarquee(X);
	if (!X.S.HasResults)
	{
		return;
	}
	const Results& R = X.S.LastResults;
	const std::string Key = R.EventName + "#" + std::to_string(static_cast<long long>(X.S.ResultsAt * 1000.0));
	if (X.U.ResultKey != Key)
	{
		X.U.ResultKey = Key;
		X.U.ResultAt = X.Now;
	}
	const double Since = X.Preview ? 30.0 : X.Now - X.U.ResultAt;
	const bool Seat = !R.SeatWon.empty();
	const Chips Prize = R.PrizeCents + R.BountyCents;
	const bool Cashed = Prize > 0 || Seat;
	// The stage card springs in (ease-out-back).
	const float Spring = IgniteBack(Since / 0.35);
	const Rect Card{300.0f, 96.0f, 1000.0f, 776.0f};
	C.Save();
	C.Translate(800.0f, 484.0f);
	C.Scale(0.85f + 0.15f * Spring, 0.85f + 0.15f * Spring);
	C.Translate(-800.0f, -484.0f);
	C.GlowRoundRect({Card.X, Card.Y + 12.0f, Card.W, Card.H}, 30.0f, Rgba(0, 0, 0, 0.5f), 30.0f);
	C.FillRoundRect(Card, 30.0f, Paint::Linear({0.0f, Card.Y}, {0.0f, Card.Y + Card.H}, IgniteHx(0x3a1f5c), IgniteHx(0x1d1030)));
	IgniteNeonRoundRect(C, Card, 30.0f, R.Won ? Hex(IgSun) : Hex(K.Coral), 2.5f, 8.0f);
	// The placement: a medal for the podium, else "28TH OF 410"; YOU WON with fireworks for a win.
	{
		const Color Medal = R.Place == 1 ? Hex(IgSun) : R.Place == 2 ? Hex(0xd9d4e8) : Hex(0xe0905a);
		if (R.Place >= 1 && R.Place <= 3 && !Seat)
		{
			C.GlowRoundRect({370.0f, 120.0f, 100.0f, 100.0f}, 50.0f, IgniteA(Medal, 0.4f), 20.0f);
			IgniteSticker(C, 420.0f, 170.0f, 44.0f, std::to_string(R.Place), Medal, IgniteHx(0x2a1640), 40.0f, 16, 0.8f, 0.6f + 0.4f * Spring);
		}
		const std::string Place = Seat ? std::string("SEAT WON") : R.Won ? std::string("YOU WON") : IgniteUpper(Ordinal(R.Place)) + " OF " + Grouped(R.Entrants);
		C.Text(Place, R.Place >= 1 && R.Place <= 3 && !Seat ? 490.0f : 350.0f, 186.0f, IgniteBi(48.0f, R.Won || Seat ? Hex(IgSun) : Hex(K.Cream)));
		C.Text(R.EventName + " \xC2\xB7 buy-in " + (R.BuyInCents ? Money(R.BuyInCents) : std::string("free")), R.Place >= 1 && R.Place <= 3 && !Seat ? 492.0f : 352.0f, 214.0f,
			IgniteTx(16.0f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 760.0f));
		if (R.Won || Seat)
		{
			motion::Confetti(C, Fnv1a(Key), X.U.ResultAt, X.Preview ? X.U.ResultAt + 100.0 : X.Now, 160, {300.0f, 96.0f, 1000.0f, 500.0f});
			if (Seat)
			{
				// A seat has no count-up to land: its boom is now. A cash's boom lands with the prize (below), once.
				IgniteCue(X.S, X.U, Key + ":won", SoundId::IgBoom, 0.6f, X.Preview);
			}
		}
	}
	// The prize, counting up over 0.9 s with an odometer step and a tick a step (IgSpark), landing with IgBoom.
	{
		if (Seat)
		{
			C.Text("A " + Money(R.SeatValueCents) + " ticket", 350.0f, 330.0f, IgniteBi(60.0f, Hex(IgSun), Align::Left, Baseline::Alphabetic, 900.0f));
		}
		else if (Prize > 0)
		{
			const double Shown = motion::CountUp(0.0, static_cast<double>(Prize), X.Preview ? 1.0e9 : X.Now, X.U.ResultAt + 0.3, 0.9);
			const Chips Stepped = static_cast<Chips>(std::floor(Shown / static_cast<double>(std::max<Chips>(1, Prize / 12)))) * std::max<Chips>(1, Prize / 12);
			const Chips Face = Shown >= static_cast<double>(Prize) ? Prize : std::min(Prize, Stepped);
			C.Text("+" + Money(Face), 350.0f, 336.0f, IgniteBi(96.0f, Hex(IgSun), Align::Left, Baseline::Alphabetic, 900.0f));
			const int Step = motion::Stage(X.Now, X.U.ResultAt + 0.3, {0.15, 0.3, 0.45, 0.6, 0.75});
			for (int I = 1; I <= Step; ++I)
			{
				IgniteCue(X.S, X.U, Key + ":tick" + std::to_string(I), SoundId::IgSpark, 0.3f, X.Preview);
			}
			if (Since >= 1.2)
			{
				IgniteCue(X.S, X.U, Key + ":land", SoundId::IgBoom, R.Won ? 0.6f : 0.4f, X.Preview);
			}
			if (R.BountyCents > 0)
			{
				C.Text(Money(R.BountyCents) + " in bounties \xC2\xB7 " + std::to_string(R.Knockouts) + (R.Knockouts == 1 ? " knockout" : " knockouts"), 352.0f, 366.0f, IgniteTx(17.0f, 800, Hex(IgTang)));
			}
		}
		else
		{
			C.Text(IgniteNoCash, 350.0f, 320.0f, IgniteBi(48.0f, pal::Muted));
		}
	}
	// The balance the result moved (one run: the results test reads it), whose money it was, the sitting.
	{
		const std::string Split = riverline_detail::ResultSplit(X.S, R);
		float Y = 404.0f;
		if (!Split.empty())
		{
			C.Text(Split, 352.0f, Y, IgniteTx(15.0f, 700, Hex(IgTang), Align::Left, Baseline::Alphabetic, 900.0f));
			Y += 28.0f;
		}
		const float Bw = C.Text("Ignite balance now " + Money(rlsites_detail::SitesBalance(X.S, net::SiteId::Ignite)), 352.0f, Y, IgniteTx(20.0f, 700, Hex(K.Cream)));
		if (R.SessionEvents > 1)
		{
			const std::string Net = (R.SessionNetCents >= 0 ? "+" : "") + Money(R.SessionNetCents);
			const float Lw = C.Text("This sitting: " + std::to_string(R.SessionEvents) + " tournaments \xC2\xB7 net ", 376.0f + Bw, Y, IgniteTx(15.0f, 600, pal::Muted));
			C.Text(Net, 376.0f + Bw + Lw, Y, IgniteTx(15.0f, 800, R.SessionNetCents >= 0 ? Hex(IgLime) : pal::Red));
		}
	}
	// How you played it: the decision accuracy (your own decisions only), the XP, the grades and the worst three spots.
	{
		const float Gx = 440.0f;
		const float Gy = 560.0f;
		const double Acc = R.AccuracyPct;
		const double Dial = Acc * (X.Preview ? 1.0 : EaseOutCubic(Clamp01((X.Now - X.U.ResultAt - 0.4) / 1.2)));
		C.StrokeArc(Gx, Gy, 78.0f, Pi * 0.75f, Pi * 2.25f, IgniteHx(K.Well), 16.0f, true);
		if (Dial > 0.1)
		{
			const Color Gc = Acc >= 85.0 ? Hex(IgLime) : Acc >= 70.0 ? Hex(K.Sky) : Acc >= 55.0 ? Hex(IgSun) : Hex(IgRed);
			C.StrokeArc(Gx, Gy, 78.0f, Pi * 0.75f, Pi * 0.75f + Pi * 1.5f * static_cast<float>(Dial) / 100.0f, Gc, 16.0f, true);
		}
		C.Text(R.Grades.empty() ? std::string("\xE2\x80\x94") : Fixed(Acc, 1), Gx, Gy + 12.0f, IgniteBi(38.0f, Hex(K.Cream), Align::Center));
		C.Text("Decision accuracy", Gx, Gy + 46.0f, IgniteTx(13.0f, 600, pal::Muted, Align::Center));
		const autoplay::Played* PlanRec = rlauto_detail::AutoPlayedFor(X.S, R);
		const autoplay::Played* ByPlan = X.Preview ? nullptr : PlanRec; // never on a stream (U1)
		const double OwnXp = static_cast<double>(R.Grades.size()) * (Acc / 100.0) * 12.0;
		const int Xp = static_cast<int>(JsRound(OwnXp + (Cashed && !PlanRec ? 60.0 : 0.0)));
		C.Text(PlanRec && R.Grades.empty() ? std::string("No skill XP") : "SKILL XP +" + std::to_string(Xp), Gx, Gy + 90.0f, IgniteBi(20.0f, Hex(K.Coral), Align::Center));
		C.Text(ByPlan ? rlauto_detail::AutoXpOwn : IgniteDecisionsXp, Gx, Gy + 114.0f, IgniteTx(12.0f, 500, pal::Muted, Align::Center, Baseline::Alphabetic, 260.0f));
		if (ByPlan)
		{
			C.Text(autoplay::PlayedLine(*ByPlan), Gx, Gy + 136.0f, IgniteTx(12.5f, 700, Hex(K.Sky), Align::Center, Baseline::Alphabetic, 280.0f));
		}
		// The grades: a candy bar of the five, then the three costliest spots.
		const Grade Order[5] = {Grade::Best, Grade::Good, Grade::Inaccuracy, Grade::Mistake, Grade::Blunder};
		const uint32_t GradeCols[5] = {IgLime, K.Sky, IgSun, IgTang, IgRed};
		int Counts[5] = {0, 0, 0, 0, 0};
		for (const DecisionGrade& G : R.Grades)
		{
			++Counts[static_cast<int>(G.Result)];
		}
		const float Total = static_cast<float>(R.Grades.empty() ? 1 : R.Grades.size());
		float Bx = 600.0f;
		for (int I = 0; I < 5; ++I)
		{
			const float W = static_cast<float>(Counts[I]) / Total * 640.0f;
			if (W > 0.0f)
			{
				C.FillRoundRect({Bx, 478.0f, std::max(6.0f, W - 3.0f), 18.0f}, 9.0f, Hex(GradeCols[I]));
			}
			Bx += W;
			C.Text(std::string(GradeName(Order[I])) + " " + std::to_string(Counts[I]), 600.0f + static_cast<float>(I) * 130.0f, 520.0f, IgniteTx(13.0f, 800, Hex(GradeCols[I])));
		}
		std::vector<DecisionGrade> Worst;
		for (const DecisionGrade& G : R.Grades)
		{
			if (G.Result != Grade::Best && G.Result != Grade::Good)
			{
				Worst.push_back(G);
			}
		}
		std::stable_sort(Worst.begin(), Worst.end(), [](const DecisionGrade& A, const DecisionGrade& B) { return A.LossBB > B.LossBB; });
		C.Text("HAND REVIEW", 600.0f, 562.0f, IgniteBi(16.0f, Hex(K.Cream)));
		if (Worst.empty())
		{
			C.Text(R.Grades.empty() ? (ByPlan ? rlauto_detail::AutoEveryDecision : IgniteNoGrades) : IgniteCleanGame, 600.0f, 592.0f, IgniteTx(14.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, 640.0f));
		}
		for (size_t I = 0; I < Worst.size() && I < 3; ++I)
		{
			const DecisionGrade& G = Worst[I];
			const float Y = 578.0f + static_cast<float>(I) * 50.0f;
			C.FillRoundRect({600.0f, Y, 660.0f, 42.0f}, 14.0f, IgniteHx(K.Well));
			C.Text(GradeName(G.Result), 616.0f, Y + 27.0f, IgniteTx(14.0f, 800, Hex(GradeCols[static_cast<int>(G.Result)])));
			C.Text(std::string(StreetTitle(G.OnStreet)) + " \xC2\xB7 you chose " + rlnet_detail::NetLower(G.Chosen.Label), 736.0f, Y + 27.0f, IgniteTx(13.5f, 600, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 240.0f));
			C.Text(G.Note, 1244.0f, Y + 27.0f, IgniteTx(12.5f, 500, pal::Muted, Align::Right, Baseline::Alphabetic, 260.0f));
		}
	}
	// Play again (the next start of the same family, through the sheet and its strike gate) and back to PLAY.
	{
		net::EventInstance Next;
		const bool Again = !X.S.History.empty() && X.S.History.front().Name == R.EventName && IgniteAgain(X.S.History.front().EventId, X.World, Next);
		const rlsites_detail::SiteLook& Look = rlsites_detail::SitesLook(net::SiteId::Ignite);
		ButtonOpts Go;
		Go.Kind = ButtonKind::Primary;
		Go.Size = 19.0f;
		Go.Enabled = Again && !X.Preview;
		Go.Sub = Again ? net::ForEvent(Next.Id).TemplateOf(Next).Name + " \xC2\xB7 " + IgniteWhen(Next.Start, X.World) : std::string("Nothing like it on the floor right now");
		if (rlsites_detail::SitesButton(UI, C, "again", {352.0f, 776.0f, 440.0f, 72.0f}, "Play again", Go, Look) && Again)
		{
			X.S.LeaveResults();
			X.S.ShowLobby();
			Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
			IgniteOpenSheet(X.RL, Next.Id, X.Now);
		}
		ButtonOpts Back;
		Back.Kind = ButtonKind::Secondary;
		Back.Size = 19.0f;
		Back.Enabled = !X.Preview;
		if (rlsites_detail::SitesButton(UI, C, "backlobby", {808.0f, 776.0f, 440.0f, 72.0f}, "Back to play", Back, Look))
		{
			X.S.LeaveResults();
		}
	}
	C.Restore();
}

// ------------------------------------------------------------------ at the tables: the marquee over the shared table

void IgniteTables(IgniteCtx& X)
{
	RiverLine& RL = X.RL;
	IgniteMarquee(X);
	// A player card (a name at the table caught the player's eye) holds the table's input while it's up, and so does the
	// autoplay panel (update 2 §2.6.1), as in RiverLine's frame.
	const bool Card = !X.Preview && Access::CardOpen(RL);
	const bool Auto = !X.Preview && !Card && rlauto_detail::AutoPanelOpen(&RL);
	const Pointer Held = X.UI.Ptr;
	if (Card || Auto)
	{
		X.UI.Ptr.Pressed = false;
		X.UI.Ptr.Released = false;
		X.UI.Ptr.Wheel = 0.0f;
		X.UI.Ptr.Active = false;
	}
	const bool Tiled = X.S.Tiled && X.S.TableCount() >= 2;
	if (Tiled)
	{
		Access::Tiles(RL, X.Now);
	}
	else
	{
		Access::Table(RL, X.Now);
	}
	if (Card)
	{
		X.UI.Ptr = Held;
		Access::PlayerCard(RL, X.Now);
	}
	else if (Auto)
	{
		X.UI.Ptr = Held;
		const float Pw = 1108.0f;
		const float Cx = rlsites_detail::SitesTableLook(X.S).Cx;
		Access::AutoPanel(RL, {Tiled ? (RiverLine::Width - Pw) / 2.0f : Cx - Pw / 2.0f, IgniteMarqueeH + 6.0f, Pw, 870.0f}, X.Now);
	}
}

IgniteHop IgniteHopAt(double Since, double Dur, int K, uint32_t Seed)
{
	// The chips leave the pot 35 ms apart and the last lands as the pot's flight does; each flies a parabola 70-104 units
	// high, fanning up to 11 units to a side, then bounces once (10 units, 0.18 s) and settles into the stack (0.12 s).
	IgniteHop H;
	constexpr double Stagger = 0.035;
	const double Fly = std::max(0.2, Dur - Stagger * static_cast<double>(IgniteFountainChips - 1));
	const double T = Since - Stagger * static_cast<double>(K);
	if (T < 0.0 || T >= Fly + 0.3)
	{
		return H;
	}
	const uint32_t Mix = igniteart::IgniteMix(Seed + static_cast<uint32_t>(K) * 0x9E3779B9u);
	const float Apex = 70.0f + 34.0f * static_cast<float>(Mix & 0xffu) / 255.0f;
	const float Fan = (static_cast<float>((Mix >> 8) & 0xffu) / 255.0f - 0.5f) * 22.0f;
	const float U = static_cast<float>(Clamp01(T / Fly));
	H.Shown = true;
	H.Along = U;
	H.Spread = Fan * U;
	H.Lift = 4.0f * Apex * U * (1.0f - U);
	if (T >= Fly)
	{
		const float V = static_cast<float>((T - Fly) / 0.18);
		H.Lift = V < 1.0f ? 10.0f * 4.0f * V * (1.0f - V) : 0.0f;
		H.Alpha = V < 1.0f ? 1.0f : static_cast<float>(Clamp01(1.0 - (T - Fly - 0.18) / 0.12));
	}
	return H;
}
} // namespace rlignite_detail

void RiverLine::IgniteFrame(double Now, bool Preview)
{
	rlignite_detail::IgniteUi& U = rlignite_detail::IgniteOf(this);
	if (U.Owner != &S)
	{
		U = rlignite_detail::IgniteUi();
		U.Owner = &S;
	}
	const site::show::Mood Mood = site::show::IgniteMood(World);
	rlignite_detail::IgniteCtx X{*this, S, *C, UI, U, igniteart::IgniteTonesFor(Mood)};
	X.Now = Now;
	X.Tm = Preview ? rlignite_detail::IgniteStill : Now;
	X.World = World;
	X.Dt = Dt;
	X.Preview = Preview;
	// The autoplay panel belongs to the tables (update 2 §2.6.1), as in RiverLine's frame.
	if (!Preview && S.CurrentScreen != Screen::Table && rlauto_detail::AutoPanelOpen(this))
	{
		rlauto_detail::AutoClosePanel(this);
	}
	switch (S.CurrentScreen)
	{
	case Screen::Lobby: rlignite_detail::IgniteLobby(X); break;
	case Screen::Table: U.Sheet = false; rlignite_detail::IgniteTables(X); break;
	case Screen::Results: U.Sheet = false; rlignite_detail::IgniteResults(X); break;
	case Screen::Boot: HomeFrame(Now, Preview); return;
	}
	if (!Preview)
	{
		FinishedToasts(Now);
	}
}

// ------------------------------------------------------------------ the capsule table's layer (SPEC §2.5)

void RiverLine::IgniteTableLayer(double Now)
{
	// Over the shared table, which draws in Ignite's SiteLook (ui-tablestyle's: the capsule in its neon tube, the bubbles with
	// the fuse burning round the acting ring and the action stickers, the candy chips, the Sunset back, the sunflower
	// button): the layer adds the table's moments. The KO sticker by a bounty's line, the win's confetti, and the cues of the
	// stickers and the fuse the bubbles draw (a slam as an ALL IN lands, the crackle as the hero's fuse burns into its last
	// 5 s), each once. G2 adds the pins (a Headliner's star, your Beef), SCALP and PAYBACK and the Ouch bubble. While
	// previewing it draws a still and plays nothing.
	using namespace igniteart;
	rlignite_detail::IgniteUi& U = rlignite_detail::IgniteOf(this);
	const bool Still = Previewing;
	const bool HandOver = S.CurHand && S.CurHand->bComplete;
	for (const SeatVis& Seat : S.Seats)
	{
		if (!Seat.Present)
		{
			continue;
		}
		if (Seat.Acting && !HandOver && Seat.IsHero)
		{
			const bool Bank = S.HasPrompt && S.Prompt.TimeBankUntil > 0.0;
			const double Left = Bank ? S.Prompt.TimeBankUntil - Now : Seat.ActEnd - Now;
			if (Left < 5.0 && Left > 0.0)
			{
				rlignite_detail::IgniteCue(S, U, "fuse:" + Seat.Id + ":" + std::to_string(static_cast<long long>(Seat.ActStart * 100.0)), SoundId::IgFuse, 0.35f, Still);
			}
		}
		if (Seat.LastAction == "All-in" && Now - Seat.LastActionAt < 0.6 && Now >= Seat.LastActionAt)
		{
			rlignite_detail::IgniteCue(S, U, "slam:" + Seat.Id + ":" + std::to_string(static_cast<long long>(Seat.LastActionAt * 100.0)), SoundId::IgPop, 0.45f, Still);
		}
		// A win worth a moment: the hero's all-in held up at showdown. The default win effect, confetti (≤ 160 pieces, its
		// paths hashed from the hand), over the felt.
		if (Seat.IsHero && Seat.Winner && Seat.AllIn && HandOver)
		{
			const std::string Key = "win:" + std::to_string(static_cast<long long>(Seat.DealtAt * 100.0));
			auto It = U.Stickers.find(Key);
			if (It == U.Stickers.end())
			{
				It = U.Stickers.emplace(Key, Now).first;
				if (U.Stickers.size() > 64)
				{
					U.Stickers.clear();
					It = U.Stickers.emplace(Key, Now).first;
				}
			}
			if (!Still)
			{
				const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
				motion::Confetti(*C, Fnv1a(Key), It->second, Now, 140, {Look.Cx - 440.0f, Look.Cy - 220.0f, 880.0f, 440.0f});
			}
			rlignite_detail::IgniteCue(S, U, Key, SoundId::IgBoom, 0.45f, Still);
		}
	}
	// A knockout in a bounty event: the KO sticker by the bounty's line (the shared table writes it beside the player).
	if (Now - S.LastBountyAt < 2.6 && S.LastBountyAt > 0.0)
	{
		const Vec2 Hp = SlotPos(HeroSeatNo());
		const float Pop = Still ? 1.0f : static_cast<float>(motion::Spring(Now, S.LastBountyAt));
		IgniteSticker(*C, Hp.X + 110.0f, Hp.Y - 70.0f, 26.0f, "KO", Hex(IgRed), Hex(IgDarkInk), 17.0f, 14, 0.8f, Pop);
		rlignite_detail::IgniteCue(S, U, "ko:" + std::to_string(static_cast<long long>(S.LastBountyAt * 100.0)), SoundId::IgPop, 0.5f, Still);
	}
	// A pot won (SPEC §2.5 "Bets and pots", G1_AUDIT L1): its chips fountain out of the pot to the winner, each on its own
	// parabola, bouncing once where it lands, a trail of six sparklets behind the first; then the pot's size floats up beside
	// the winner's bubble ("+4,200"). The pot's flight (Session's Award) is gone 0.05 s after it lands, so the layer keeps
	// each one it sees. A still while previewing draws none of it.
	for (const Flight& F : S.Flights)
	{
		if (F.IsChips && F.Amount > 0 && F.From.Type == FlightEnd::Kind::Pot && F.To.Type == FlightEnd::Kind::Seat)
		{
			const std::string Key = std::to_string(F.To.Seat) + "@" + std::to_string(static_cast<long long>(std::llround(F.Start * 1000.0)));
			if (U.Pots.find(Key) == U.Pots.end())
			{
				U.Pots[Key] = {F.To.Seat, F.Amount, F.Start, F.Dur};
			}
		}
	}
	for (auto It = U.Pots.begin(); It != U.Pots.end();)
	{
		const rlignite_detail::IgniteUi::PotWin& P = It->second;
		It = Now > P.Start + P.Dur + rlignite_detail::IgniteFloatFor || Now < P.Start - 1.0 ? U.Pots.erase(It) : std::next(It);
	}
	if (!Still)
	{
		for (const auto& Entry : U.Pots)
		{
			const rlignite_detail::IgniteUi::PotWin& P = Entry.second;
			const Vec2 From = FlightPoint(FlightEnd::AtPot());
			const Vec2 To = FlightPoint(FlightEnd::AtSeat(P.Seat));
			const uint32_t Seed = Fnv1a(Entry.first);
			for (int K = 0; K < rlignite_detail::IgniteFountainChips; ++K)
			{
				const rlignite_detail::IgniteHop H = rlignite_detail::IgniteHopAt(Now - P.Start, P.Dur, K, Seed);
				if (!H.Shown)
				{
					continue;
				}
				const float X = From.X + (To.X - From.X) * H.Along + H.Spread;
				const float Y = From.Y + (To.Y - From.Y) * H.Along - H.Lift;
				if (K == 0 && H.Along < 1.0f)
				{
					// The trail: six sparklets back along the first chip's arc, smaller and fainter the further back.
					for (int J = 1; J <= 6; ++J)
					{
						const rlignite_detail::IgniteHop Back = rlignite_detail::IgniteHopAt(Now - P.Start - 0.03 * static_cast<double>(J), P.Dur, K, Seed);
						if (!Back.Shown || Back.Along <= 0.0f)
						{
							continue;
						}
						const float Bx = From.X + (To.X - From.X) * Back.Along + Back.Spread;
						const float By = From.Y + (To.Y - From.Y) * Back.Along - Back.Lift;
						IgniteSpark(*C, Bx, By, 5.0f - 0.55f * static_cast<float>(J), IgniteA(Hex(J % 2 == 0 ? IgPink : IgSun), 0.9f - 0.12f * static_cast<float>(J)));
					}
				}
				IgniteCandyChip(*C, X, Y, 8.5f, static_cast<int>((Seed + static_cast<uint32_t>(K)) % 6u), H.Alpha);
			}
			// "+4,200": the pot's chips, floating up beside the winner once they've landed.
			const double Since = Now - (P.Start + P.Dur);
			if (Since >= 0.0 && Since < rlignite_detail::IgniteFloatFor)
			{
				const Vec2 Seat = SlotPos(P.Seat);
				const float Rise = static_cast<float>(EaseOutCubic(Since / rlignite_detail::IgniteFloatFor));
				const float Fade = static_cast<float>(Clamp01((rlignite_detail::IgniteFloatFor - Since) / 0.45));
				const float Pop = static_cast<float>(motion::Spring(Now, P.Start + P.Dur));
				const float A0 = C->GetAlpha();
				C->SetAlpha(A0 * Fade);
				C->Save();
				// Beside the bubble on its left, rising 40: clear of the seat's cards (above it on its right) and its name pill
				// (under it). A seat by the screen's left edge (8-max's (140,430)) whose number wouldn't fit there, spring's
				// peak included, takes it on its right, past its cards.
				const std::string Won = "+" + Grouped(P.Amount);
				const bool OnLeft = Seat.X - 44.0f - C->Measure(Won, 28.0f, Font::BlackItalic) * 1.1f >= 8.0f;
				C->Translate(OnLeft ? Seat.X - 44.0f : Seat.X + 62.0f, std::max(rlignite_detail::IgniteMarqueeH + 30.0f, Seat.Y + 8.0f - 40.0f * Rise));
				C->Scale(Pop, Pop);
				C->Text(Won, 1.5f, 2.0f, IgniteBi(28.0f, Rgba(0, 0, 0, 0.55f), OnLeft ? Align::Right : Align::Left));
				C->Text(Won, 0.0f, 0.0f, IgniteBi(28.0f, Hex(IgSun), OnLeft ? Align::Right : Align::Left));
				C->Restore();
				C->SetAlpha(A0);
			}
		}
	}
}

namespace rlignite_detail
{
// ------------------------------------------------------------------ the capsule table's side panel and pot (G1_AUDIT N3-N5)

namespace
{
/** Low-poly pieces for the panel's many small shapes (the canvas sizes an arc by its radius; a panel of a dozen bubbles needs
 * fewer): a round rect at 3 segments a corner (33 vertices; 57 at the canvas's), a disc of N points (2N + 1). */
void IgniteLowRoundRect(Canvas& C, const Rect& R, float Radius, const Paint& P)
{
	C.FillPolygon(Canvas::RoundRectPath(R, Radius, 3), P);
}

void IgniteLowDisc(Canvas& C, float X, float Y, float R, int N, const Paint& P)
{
	C.FillPolygon(Canvas::EllipsePath(X, Y, R, R, 0.0f, 2.0f * Pi, N, false), P);
}

/** A name's first character, whole (a UTF-8 sequence kept together), a Latin letter in capitals. */
std::string IgniteInitial(const std::string& Name)
{
	if (Name.empty())
	{
		return "?";
	}
	const unsigned char Lead = static_cast<unsigned char>(Name[0]);
	if (Lead < 0x80)
	{
		return std::string(1, static_cast<char>(std::toupper(Lead)));
	}
	const size_t Len = (Lead >> 5) == 0x6u ? 2 : (Lead >> 4) == 0xEu ? 3 : (Lead >> 3) == 0x1Eu ? 4 : 1;
	return Name.substr(0, std::min(Len, Name.size()));
}

/** A bubble avatar: a disc in the speaker's light (12 points) with the initial in plum ink (BlackItalic). */
void IgniteChatAvatar(Canvas& C, float X, float Y, float R, const std::string& Name, const Color& Light)
{
	IgniteLowDisc(C, X, Y, R, 12, Light);
	TextStyle In = Ts(R, 900, Hex(rlsites_detail::SitesIgPlumInk), Align::Center, Baseline::Middle);
	In.Face = static_cast<int8_t>(Font::BlackItalic);
	C.Text(IgniteInitial(Name), X, Y + 0.5f, In);
}

/** Words to lines of at most MaxW (TblWrap's rule: a word too wide for a line keeps a line of its own). */
std::vector<std::string> IgniteWrap(const Canvas& C, const std::string& Text, float MaxW, float Size, Font Face)
{
	std::vector<std::string> Lines;
	std::string Line;
	size_t At = 0;
	while (At <= Text.size())
	{
		const size_t Sp = Text.find(' ', At);
		const std::string Word = Text.substr(At, Sp == std::string::npos ? std::string::npos : Sp - At);
		const std::string Try = Line.empty() ? Word : Line + " " + Word;
		if (!Line.empty() && C.Measure(Try, Size, Face) > MaxW)
		{
			Lines.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Try;
		}
		if (Sp == std::string::npos)
		{
			break;
		}
		At = Sp + 1;
	}
	if (!Line.empty())
	{
		Lines.push_back(Line);
	}
	return Lines;
}

/** A key for a chat item's fit and its cached geometry (FNV-1a, 64-bit, over everything the piece draws). */
uint64_t IgniteKeyMix(uint64_t K, uint64_t V)
{
	return (K ^ V) * 0x100000001b3ull;
}

uint64_t IgniteKeyText(uint64_t K, const std::string& S)
{
	for (const char Ch : S)
	{
		K = IgniteKeyMix(K, static_cast<uint8_t>(Ch));
	}
	return IgniteKeyMix(K, 0x1ffu);
}

uint64_t IgniteKeyFloat(uint64_t K, float V)
{
	uint32_t U = 0;
	std::memcpy(&U, &V, sizeof(U));
	return IgniteKeyMix(K, U);
}

uint64_t IgniteKeyColor(uint64_t K, const Color& Col)
{
	for (const float V : {Col.R, Col.G, Col.B, Col.A})
	{
		K = IgniteKeyFloat(K, V);
	}
	return K;
}

/** "36.5 bb" (the seats' and the head's stack, TblBbLower's). */
std::string IgniteBbLower(Chips Amount, Chips BigBlind)
{
	const double Bb = static_cast<double>(Amount) / static_cast<double>(std::max<Chips>(1, BigBlind));
	return Fixed(Bb, Bb < 100.0 ? 1 : 0) + " bb";
}
} // namespace

Color IgniteNameLight(const std::string& Name, bool Rival, bool Hero)
{
	if (Hero)
	{
		return pal::Accent;
	}
	if (Rival)
	{
		return Hex(igniteart::IgRed);
	}
	static const uint32_t Lights[6] = {igniteart::IgPink, igniteart::IgSky, igniteart::IgTang, igniteart::IgLime, igniteart::IgSun, igniteart::IgViolet};
	uint32_t H = 2166136261u;
	for (const char Ch : Name)
	{
		H = (H ^ static_cast<uint8_t>(Ch)) * 16777619u;
	}
	return Hex(Lights[H % 6u]);
}

void IgniteTableChat(Canvas& C, const RiverLine* RL, const std::vector<ChatLine>& Chat, const Rect& Box)
{
	// G1_AUDIT N3: a chat full of bubbles was ~13 bubbles at ~98 vertices, which took the table's busiest frame (an all-in won
	// in a bounty: confetti, KO, the fountain) past its 22k. Now the newest IgniteChatBubbles items are bubbles (33 + 25
	// vertices, low-poly), the older ones above them plain lines that fade (text only), and every item's wraps are measured
	// once and its geometry replays from the Boardwalk pool (Ignite's own, least-recently-used: never the shared flush).
	IgniteUi& U = IgniteOf(RL);
	if (U.ChatFits.size() > 600)
	{
		U.ChatFits.clear();
	}
	++U.ChatFrames;
	U.ChatLines = 0;
	U.ChatBubbles = 0;
	const Color Ground = pal::Panel2;
	const Color Ink = pal::Ink;
	const Color Muted = pal::Muted;
	const float Px = C.PixelScale();
	const auto FitOf = [&U](uint64_t Key) -> IgniteUi::ChatFit* {
		const auto It = U.ChatFits.find(Key);
		return It == U.ChatFits.end() ? nullptr : &It->second;
	};
	float Y = Box.Y + Box.H - 6.0f;
	int Bubbles = 0;
	int Plain = 0;
	for (size_t Ii = Chat.size(); Ii-- > 0 && Y > Box.Y;)
	{
		const ChatLine& Ln = Chat[Ii];
		if (Ln.Kind == ChatKind::Dealer)
		{
			// The dealer's running lines: small and bare.
			const uint64_t Key = IgniteKeyFloat(IgniteKeyText(IgniteKeyText(0xcbf29ce484222325ull, "dealer"), Ln.Text), Box.W);
			IgniteUi::ChatFit* F = FitOf(Key);
			if (!F)
			{
				IgniteUi::ChatFit Made;
				Made.Lines = IgniteWrap(C, Ln.Text, Box.W - 24.0f, 11.5f, Font::Bold);
				Made.H = static_cast<float>(Made.Lines.size()) * 15.0f + 4.0f;
				F = &U.ChatFits.emplace(Key, std::move(Made)).first->second;
			}
			if (Y - F->H < Box.Y)
			{
				break; // g1-check2: a line the panel's top would cut in half is left off; the chat ends on a whole line
			}
			Y -= F->H;
			for (size_t J = 0; J < F->Lines.size(); ++J)
			{
				C.Text(F->Lines[J], Box.X + 12.0f, Y + 13.0f + static_cast<float>(J) * 15.0f, Ts(11.5f, 700, Muted));
			}
			Y -= 4.0f;
			++U.ChatLines;
			continue;
		}
		const bool Room = Ln.Kind == ChatKind::System || Ln.Who.empty();
		// A speaker's lines in a row (up to three) share one bubble and one avatar.
		size_t First = Ii;
		while (!Room && First > 0 && Ii - (First - 1) < 3 && Chat[First - 1].Kind == Ln.Kind && Chat[First - 1].Who == Ln.Who)
		{
			--First;
		}
		const bool AsBubble = Bubbles < IgniteChatBubbles;
		uint64_t Key = IgniteKeyText(0xcbf29ce484222325ull, AsBubble ? (Room ? "room" : "bubble") : (Room ? "room-plain" : "plain"));
		Key = IgniteKeyText(Key, Ln.Who);
		for (size_t K = First; K <= Ii; ++K)
		{
			Key = IgniteKeyText(Key, Chat[K].Text);
		}
		Key = IgniteKeyFloat(IgniteKeyFloat(Key, Box.W), Px);
		IgniteUi::ChatFit* F = FitOf(Key);
		if (!F)
		{
			IgniteUi::ChatFit Made;
			const float WrapW = AsBubble ? (Room ? Box.W - 48.0f : Box.W - 64.0f) : Box.W - 24.0f;
			const float Size = AsBubble ? (Room ? 12.0f : 13.0f) : Room ? 11.5f : 12.0f;
			for (size_t K = First; K <= Ii; ++K)
			{
				for (std::string& L : IgniteWrap(C, Chat[K].Text, WrapW, Size, Font::Bold))
				{
					Made.Lines.push_back(std::move(L));
				}
			}
			const float N = static_cast<float>(Made.Lines.size());
			if (AsBubble && !Room)
			{
				float Bw = C.Measure(Ln.Who, 10.5f, Font::Bold);
				for (const std::string& L : Made.Lines)
				{
					Bw = std::max(Bw, C.Measure(L, 13.0f, Font::Bold));
				}
				Made.W = std::min(Box.W - 40.0f, Bw + 24.0f);
				Made.H = 22.0f + N * 16.0f;
			}
			else if (AsBubble)
			{
				Made.W = Box.W;
				Made.H = N * 16.0f + 14.0f;
			}
			else
			{
				Made.H = Room ? N * 15.0f + 4.0f : 15.0f * (N + 1.0f) + 2.0f;
			}
			F = &U.ChatFits.emplace(Key, std::move(Made)).first->second;
		}
		if (!AsBubble && Y - F->H < Box.Y)
		{
			break; // g1-check2: the older plain lines end on a whole line, never one cut in half by the panel's top
		}
		Y -= F->H;
		U.ChatLines += static_cast<int>(Ii - First + 1);
		if (AsBubble)
		{
			++Bubbles;
			++U.ChatBubbles;
			const Color Light = Room ? Hex(rlsites_detail::SitesIgLime) : IgniteNameLight(Ln.Who, Ln.Kind == ChatKind::Rival, Ln.Kind == ChatKind::Hero);
			const uint64_t Draw = IgniteKeyColor(IgniteKeyColor(IgniteKeyColor(Key, Light), Ground), Ink);
			const IgniteUi::ChatFit& Fit = *F;
			const std::string& Who = Ln.Who;
			C.Cached(
				Draw, Box.X, Y,
				[&Fit, &Who, &Light, &Ground, &Ink, Room](Canvas& At) {
					if (Room)
					{
						// The room's lines (the welcome, a level): a wide grape bubble with a lime spark.
						IgniteLowRoundRect(At, {0.0f, 0.0f, Fit.W, Fit.H}, 12.0f, igniteart::IgniteA(Hex(igniteart::IgGrape), 0.7f));
						rlsites_detail::SitesSpark(At, 17.0f, Fit.H / 2.0f, 6.0f, Light);
						for (size_t J = 0; J < Fit.Lines.size(); ++J)
						{
							At.Text(Fit.Lines[J], 32.0f, 19.0f + static_cast<float>(J) * 16.0f, Ts(12.0f, 600, Ink));
						}
						return;
					}
					IgniteChatAvatar(At, 14.0f, 16.0f, 12.0f, Who, Light);
					IgniteLowRoundRect(At, {34.0f, 0.0f, Fit.W, Fit.H}, 14.0f, Ground);
					At.Text(Who, 46.0f, 14.0f, Ts(10.5f, 800, Light, Align::Left, Baseline::Alphabetic, false, Fit.W - 20.0f));
					for (size_t J = 0; J < Fit.Lines.size(); ++J)
					{
						At.Text(Fit.Lines[J], 46.0f, 30.0f + static_cast<float>(J) * 16.0f, Ts(13.0f, 600, Ink));
					}
				},
				Canvas::CachePool::Boardwalk);
			Y -= 8.0f;
		}
		else
		{
			// Older lines, above the bubbles: plain, the speaker's name in their light over the words, fading as they go up.
			const float A = std::max(0.4f, 0.85f - 0.07f * static_cast<float>(Plain));
			++Plain;
			float Ty = Y + 12.0f;
			if (!Room)
			{
				const Color Light = IgniteNameLight(Ln.Who, Ln.Kind == ChatKind::Rival, Ln.Kind == ChatKind::Hero);
				C.Text(Ln.Who, Box.X + 12.0f, Ty, Ts(10.5f, 800, igniteart::IgniteA(Light, A), Align::Left, Baseline::Alphabetic, false, Box.W - 24.0f));
				Ty += 15.0f;
			}
			else
			{
				Ty += 1.0f;
			}
			for (const std::string& L : F->Lines)
			{
				C.Text(L, Box.X + 12.0f, Ty, Ts(Room ? 11.5f : 12.0f, Room ? 700 : 600, igniteart::IgniteA(Room ? Muted : Ink, A * (Room ? 1.0f : 0.8f))));
				Ty += 15.0f;
			}
			Y -= 4.0f;
		}
		Ii = First;
	}
}

void IgniteTableStandings(Canvas& C, const Tournament& T, const Rect& Box)
{
	// G1_AUDIT N5 (SPEC §2.5 "Standings with level rings"): each player's bubble sits in a ring, the level ring's track the
	// seats wear (its arc is G2's levels), not a disc with a letter on it; yours is lit. Low-poly, so ten rows and yours stay
	// inside the table's 22k at its busiest frame.
	const Chips Bb = std::max<Chips>(1, T.CurrentLevel().Bb);
	const std::vector<const TPlayer*> St = T.Standings();
	const TPlayer& Me = T.Hero();
	const int HeroRank = Me.Busted ? 0 : T.HeroRank();
	const Color Track = Hex(rlsites_detail::SitesIgTrack); // the level ring's track (the acting seat's fuse burns round the same)
	float Y = Box.Y + 4.0f;
	const auto Standing = [&](int Rank, const TPlayer& Pl, float Ry) {
		const bool You = Pl.IsHero;
		if (You)
		{
			IgniteLowRoundRect(C, {Box.X, Ry, Box.W, 32.0f}, 16.0f, igniteart::IgniteA(Hex(rlsites_detail::SitesIgCoral), 0.2f));
		}
		TextStyle Rk = Ts(15.0f, 900, Rank <= 3 ? pal::Gold : pal::Muted, Align::Center);
		Rk.Face = static_cast<int8_t>(Font::BlackItalic);
		C.Text(std::to_string(Rank), Box.X + 18.0f, Ry + 22.0f, Rk);
		const float Cx = Box.X + 52.0f;
		const float Cy = Ry + 16.0f;
		// The ring: its track round a bubble in the player's light; yours lit in the accent round a cream bubble.
		IgniteLowDisc(C, Cx, Cy, 15.0f, 16, You ? pal::Accent : Track);
		IgniteChatAvatar(C, Cx, Cy, 10.5f, Pl.Name, You ? Hex(rlsites_detail::SitesIgCream) : IgniteNameLight(Pl.Name, Pl.Name == RivalName, false));
		C.Text(Pl.Name, Box.X + 76.0f, Ry + 21.0f, Ts(14.0f, You ? 800 : 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, Box.W - 76.0f - 92.0f));
		TextStyle Sk = Ts(15.0f, 900, pal::Ink, Align::Right);
		Sk.Face = static_cast<int8_t>(Font::BlackItalic);
		C.Text(IgniteBbLower(Pl.Stack, Bb), Box.X + Box.W - 14.0f, Ry + 22.0f, Sk);
	};
	const int Shown = static_cast<int>(std::min<size_t>(St.size(), 10));
	for (int I = 0; I < Shown && Y + 32.0f <= Box.Y + Box.H; ++I)
	{
		Standing(I + 1, *St[static_cast<size_t>(I)], Y);
		Y += 36.0f;
	}
	if (HeroRank > Shown && Y + 40.0f <= Box.Y + Box.H)
	{
		Standing(HeroRank, Me, Y + 6.0f);
	}
}

IgnitePotPulse IgnitePotPulseAt(double Since, float Kick)
{
	// Up in 80 ms, back through 1 by 0.2 s with a small dip, settled at 0.45 s; the glow fades over the same; the figure's
	// sunflower over 0.32 s.
	IgnitePotPulse P;
	if (!(Since >= 0.0) || Since >= IgnitePotPulseFor || Kick <= 0.0f)
	{
		return P;
	}
	const float K = std::min(1.0f, Kick);
	const double Bump = Since < 0.08 ? Since / 0.08 : std::exp(-(Since - 0.08) * 10.0) * std::cos((Since - 0.08) * 12.0);
	const double Tail = Clamp01((IgnitePotPulseFor - Since) / 0.08);
	P.Scale = 1.0f + 0.12f * K * static_cast<float>(Bump * Tail);
	const double Left = 1.0 - Since / IgnitePotPulseFor;
	P.Glow = 0.65f * K * static_cast<float>(Left * Left);
	P.Flash = static_cast<float>(Clamp01(1.0 - Since / 0.32));
	return P;
}

void IgniteTablePotPill(Canvas& C, RiverLine& RL, const Rect& Pill, Chips Total, double Now)
{
	// G1_AUDIT N4 (SPEC §2.5 "the pot pill pulses as it grows"): a bet, a call or the blinds going in pop the pill (the bigger
	// the step against the pot, the bigger the pop) and light the figure; a new hand starts it afresh, unpopped. At rest it's
	// the pill RiverLine.cpp drew: Night, a coral rim, the figure in BlackItalic.
	IgniteUi& U = IgniteOf(&RL);
	double Dealt = -1.0;
	for (const SeatVis& Seat : Access::S(RL).Seats)
	{
		Dealt = Seat.Present ? std::max(Dealt, Seat.DealtAt) : Dealt;
	}
	if (Dealt != U.PotHand || Total < U.PotSeen)
	{
		// A new count, no pop for it. A pot that shrank (shipped, a bet back) stops a running pop; a new deal lets it finish, so
		// the blinds' pop eases out as the cards go out rather than snapping back (review-fix2-ignite).
		if (Total < U.PotSeen)
		{
			U.PotGrewAt = -100.0;
			U.PotKick = 0.0f;
		}
		U.PotHand = Dealt;
		U.PotSeen = Total;
	}
	else if (Total > U.PotSeen)
	{
		// Grown again mid-pop (the antes and blinds go in a beat apart): the new pop rises from where the pill is, not from rest
		// (restarting at 1 dropped it back a frame; review-fix2-ignite).
		const float Up = (IgnitePotPulseAt(Now - U.PotGrewAt, U.PotKick).Scale - 1.0f) / 0.12f;
		const float Kick = static_cast<float>(std::min(1.0, 0.35 + 0.65 * static_cast<double>(Total - U.PotSeen) / static_cast<double>(std::max<Chips>(1, Total))));
		U.PotKick = std::max(Kick, std::min(1.0f, Up));
		U.PotGrewAt = Up > 0.0f ? Now - 0.08 * static_cast<double>(std::min(1.0f, Up / U.PotKick)) : Now;
		U.PotSeen = Total;
	}
	const IgnitePotPulse P = Access::Previewing(RL) ? IgnitePotPulse() : IgnitePotPulseAt(Now - U.PotGrewAt, U.PotKick);
	const float Cx = Pill.X + Pill.W / 2.0f;
	const float Cy = Pill.Y + Pill.H / 2.0f;
	const std::string Label = "POT " + ChipsText(static_cast<double>(Total));
	TextStyle Fg = Ts(17.0f, 900, P.Flash > 0.0f ? Mix(Hex(rlsites_detail::SitesIgCream), Hex(rlsites_detail::SitesIgSun), P.Flash) : Hex(rlsites_detail::SitesIgCream), Align::Center,
		Baseline::Middle);
	Fg.Face = static_cast<int8_t>(Font::BlackItalic);
	if (P.Glow > 0.004f)
	{
		C.GlowRoundRect({Pill.X - 4.0f, Pill.Y - 4.0f, Pill.W + 8.0f, Pill.H + 8.0f}, Pill.H / 2.0f + 4.0f, igniteart::IgniteA(pal::Accent, P.Glow), 14.0f);
	}
	if (P.Scale == 1.0f)
	{
		C.FillRoundRect(Pill, Pill.H / 2.0f, Hex(rlsites_detail::SitesIgNight));
		C.StrokeRoundRect(Pill, Pill.H / 2.0f, Hex(rlsites_detail::SitesIgCoral), 1.5f);
		C.Text(Label, Cx, Cy + 1.0f, Fg);
		return;
	}
	C.Save();
	C.Translate(Cx, Cy);
	C.Scale(P.Scale, P.Scale);
	const Rect Local{-Pill.W / 2.0f, -Pill.H / 2.0f, Pill.W, Pill.H};
	C.FillRoundRect(Local, Pill.H / 2.0f, Hex(rlsites_detail::SitesIgNight));
	C.StrokeRoundRect(Local, Pill.H / 2.0f, Hex(rlsites_detail::SitesIgCoral), 1.5f);
	C.Text(Label, 0.0f, 1.0f, Fg);
	C.Restore();
}

// ------------------------------------------------------------------ the copy

const std::vector<std::string>& IgniteShouts()
{
	// BUZZ's shouts (SPEC §2.4 J): flavour from Ignite's members, one every 10 world minutes. Never a result that didn't
	// happen (U5), never a number that isn't a fact, never the words for a seat played for you. {Event} and {Time} are
	// tonight's featured event; {Hero} is used only after your first Ignite cash of $100 or more; {Headliner} waits for the
	// Headliners (G2).
	static const std::vector<std::string> Lines = {
		"Evening, boardwalk!",
		"Good luck everyone, may your flops be kind.",
		"Who brought snacks to the pier?",
		"The neon's extra bright tonight.",
		"Back again. The arcade missed me.",
		"Just here for the bulbs and the bad beats.",
		"This lobby looks like a carnival and I love it.",
		"Somebody turn the music up!",
		"Coffee in one hand, mouse in the other.",
		"Good luck to everyone at the tables tonight.",
		"Pier's packed. Love to see it.",
		"Can we get a little sea breeze in here?",
		"New shoes, same old luck.",
		"First table of the night, wish me focus.",
		"The Ferris wheel is hypnotic, I can't stop staring.",
		"Late night crew, check in!",
		"Lucky socks on. Let's go.",
		"Sat down for one game. Famous last words.",
		"The prize counter's ticket strings look delicious.",
		"My cat is watching me play. No pressure.",
		"Who else is playing from the couch?",
		"Boardwalk vibes, poker brain.",
		"Rain outside, neon inside.",
		"Just finished dinner, ready to flip.",
		"Shout out to everyone grinding before work.",
		"Headphones on, world off.",
		"If you hear yelling, it's me folding again.",
		"One more table, then bed. Probably.",
		"The bulbs are dancing again.",
		"This is my happy place.",
		"Who's in {Event} tonight?",
		"{Event} at {Time}. Saving my energy.",
		"Registering for {Event} the second it opens.",
		"See you all in {Event}!",
		"{Event} is the only thing on my calendar.",
		"Is {Event} filling up yet?",
		"Setting an alarm for {Time}. Priorities.",
		"{Event} or bust. Hopefully not bust.",
		"Warming up before {Event}.",
		"My whole evening revolves around {Event}.",
		"Anybody else nervous about {Event}?",
		"Bringing my A game to {Event}.",
		"{Time} can't come fast enough.",
		"Saving my best poker face for {Event}.",
		"{Event} crew, where you at?",
		"Last stop before {Event}: snacks.",
		"If I'm quiet, I'm in {Event}.",
		"Clear the deck, {Event} starts at {Time}.",
		"Practicing my patience for {Event}.",
		"{Event} tonight. Deep breaths.",
		"Promised myself one entry in {Event}. One!",
		"Who wants to sit next to me in {Event}?",
		"The lights always look better before {Event}.",
		"Counting down to {Time} like it's a holiday.",
		"Told my roommate I'm busy at {Time}. Very busy.",
		"{Event} better be kind to me.",
		"Getting my seat for {Event} early, see you there.",
		"Somebody save me a seat in {Event}.",
		"{Event} at {Time}, the boardwalk's own main stage.",
		"I'm not superstitious, but I'm wearing the hat for {Event}.",
		"Who IS {Hero}?",
		"{Hero} again??",
		"Keep an eye on {Hero} tonight.",
		"{Hero} is on the floor. Tighten up, people.",
		"{Hero} cashed big here. I was paying attention.",
		"Somebody tell {Hero} to share the luck.",
		"Hope {Hero} isn't at my table tonight.",
		"{Hero} has fans now? Fine, me too.",
		"Is {Hero} streaming? Asking for a friend.",
		"{Hero} makes it look easy.",
		"Not {Hero} in the lobby again.",
		"{Hero} is the reason I study.",
		"The boardwalk knows {Hero} now.",
		"The crowd's been talking about {Hero}.",
		"Okay {Hero}, leave some for the rest of us.",
		"{Headliner} is headlining tonight, go get them.",
		"Anyone ever knocked out {Headliner}? Asking for my ego.",
		"{Headliner} on the Bandstand, I'm starstruck.",
		"Watching {Headliner} play is a free lesson.",
		"The spotlight on {Headliner}'s seat is so dramatic.",
		"I'd frame a screenshot of busting {Headliner}.",
		"{Headliner} looks unbeatable. Nobody is.",
		"Saving my best bluff for {Headliner}.",
		"{Headliner} on the billboard again, of course.",
		"Shout out to {Headliner}, the boardwalk's favourite.",
		"Folded the best hand again. Classic me.",
		"Why do I always lose the coin flips I care about?",
		"Pocket pairs: the most beautiful disappointment.",
		"Aces are just two cards with confidence.",
		"I call that a hero fold. Others call it scared.",
		"The river and I are not on speaking terms.",
		"Note to self: the button is not a suggestion.",
		"Turbo structure, turbo heart rate.",
		"I play tight. Tight-ish. Loose. I play.",
		"Blinds are going up and so is my blood pressure.",
		"Bounties make everyone brave.",
		"Mystery envelopes are basically gift wrap for chips.",
		"A flip is just a short story with a twist.",
		"Who decided the turn should be that card?",
		"My hand reading is perfect until the showdown.",
		"Short stack, big dreams.",
		"I respect a good shove. I fear it, but I respect it.",
		"Never underestimate a min-raise.",
		"Took a deep breath and folded. Growth.",
		"The flop gives me hope. The turn takes it back.",
		"I've never seen so many suited connectors in my life.",
		"Every table has one talker. Tonight it's me.",
		"If you limp, I will notice.",
		"Shoving is a love language.",
		"Patience is a skill. I'm still training it.",
		"Bubble time is the best time. Fight me.",
		"Small ball, big dreams.",
		"My reads are sharp tonight. Probably.",
		"That's poker, baby.",
		"Calling with ace high and no regrets.",
		"Final table dreams start with a single chip.",
		"Re-entry is a mindset.",
		"I love a good fast structure.",
		"Slow play is a lifestyle choice.",
		"Bluff catchers unite.",
		"Is it a bluff if you believe it?",
		"Breathing exercises between hands, highly recommend.",
		"The quiet ones always have it.",
		"Every chip counts until it doesn't.",
		"Defending the big blind like it's my house.",
		"Running deep is the best feeling on the pier.",
		"The bubble makes everyone polite.",
		"Late registration is my cardio.",
		"Calling the clock on my own decisions.",
		"Sometimes the best move is the snack break.",
		"Somebody's going to get lucky tonight. Why not me?",
		"Climb, little stack, climb.",
		"Hands per hour: a lot. Hands won: we'll see.",
		"I have a system. The system is vibes.",
		"Reminder: fold equity is real.",
		"Respect the button, fear the blinds.",
		"Polite nod to whoever keeps check-raising me.",
		"That turn card owes me an apology.",
		"Grinding until the bulbs go dim.",
		"Afterburner crew, assemble after midnight.",
		"Night owls play better. Science.",
		"Can't sleep, might as well flip.",
		"Morning poker hits different with coffee.",
		"Lunch break poker, the purest form.",
		"Nothing beats a Sunday on the boardwalk.",
		"Bounty hunting is my favourite sport.",
		"Mystery envelopes, please be kind.",
		"Flip and run, my favourite way to start a night.",
		"Cheap seats tonight, big dreams anyway.",
		"Guarantees on the board, hope on the pier.",
	};
	return Lines;
}

const std::vector<IgniteShoutHours>& IgniteShoutWindows()
{
	// The lines that say what time it is. Any other line with "tonight" in it is heard from 3 PM into the small hours.
	static const std::vector<IgniteShoutHours> Windows = {
		{"Evening, boardwalk!", 17, 23, false},
		{"Just finished dinner, ready to flip.", 18, 23, false},
		{"First table of the night, wish me focus.", 17, 24, false},
		{"The neon's extra bright tonight.", 18, 5, false},
		{"Late night crew, check in!", 22, 5, false},
		{"One more table, then bed. Probably.", 21, 4, false},
		{"Grinding until the bulbs go dim.", 19, 5, false},
		{"Afterburner crew, assemble after midnight.", 20, 2, false},
		{"Night owls play better. Science.", 22, 6, false},
		{"Can't sleep, might as well flip.", 0, 6, false},
		{"Coffee in one hand, mouse in the other.", 5, 12, false},
		{"Shout out to everyone grinding before work.", 5, 10, false},
		{"Morning poker hits different with coffee.", 5, 12, false},
		{"Lunch break poker, the purest form.", 11, 15, false},
		{"Nothing beats a Sunday on the boardwalk.", 8, 24, true},
	};
	return Windows;
}

bool IgniteShoutFits(const std::string& Line, double World)
{
	const int Hour = static_cast<int>(std::floor(std::fmod(World, net::MinutesPerDay) / 60.0));
	const auto Within = [Hour](int From, int To) { return From < To ? Hour >= From && Hour < To : Hour >= From || Hour < To; };
	for (const IgniteShoutHours& W : IgniteShoutWindows())
	{
		if (Line == W.Line)
		{
			return Within(W.From, W.To) && (!W.Sunday || net::Weekday(net::DayOf(World)) == 6);
		}
	}
	return rlnet_detail::NetLower(Line).find("tonight") == std::string::npos || Within(15, 5);
}

int IgniteShoutAt(uint64_t WorldSeed, double World)
{
	// One line every 10 world minutes; a 25-hour window holds 150 slots, one per line. The lines sit in fixed blocks of seven
	// (a keyed order for the world) and each window shuffles inside every block (keyed by the window), so a line moves at most
	// six slots between windows: nothing repeats within 24 world hours, across a window's edge too (rivals_ui tests it).
	const std::vector<std::string>& Lines = IgniteShouts();
	const int N = static_cast<int>(Lines.size());
	if (N == 0)
	{
		return -1;
	}
	const long long Slot = static_cast<long long>(std::floor(World / 10.0));
	const long long Window = Slot >= 0 ? Slot / N : (Slot - N + 1) / N;
	const int In = static_cast<int>(Slot - Window * N);
	const std::string Seed = std::to_string(WorldSeed) + ":ignite:shout";
	std::vector<int> Order(static_cast<size_t>(N));
	for (int I = 0; I < N; ++I)
	{
		Order[static_cast<size_t>(I)] = I;
	}
	uint32_t H = igniteart::IgniteMix(Fnv1a(Seed));
	for (int I = N - 1; I > 0; --I)
	{
		H = igniteart::IgniteMix(H + 0x9E3779B9u);
		std::swap(Order[static_cast<size_t>(I)], Order[static_cast<size_t>(H % static_cast<uint32_t>(I + 1))]);
	}
	const int Block = In / 7;
	const int First = Block * 7;
	const int Size = std::min(7, N - First);
	std::vector<int> Mine(Order.begin() + First, Order.begin() + First + Size);
	uint32_t Hw = igniteart::IgniteMix(Fnv1a(Seed + ":" + std::to_string(Window) + ":" + std::to_string(Block)));
	for (int I = Size - 1; I > 0; --I)
	{
		Hw = igniteart::IgniteMix(Hw + 0x9E3779B9u);
		std::swap(Mine[static_cast<size_t>(I)], Mine[static_cast<size_t>(Hw % static_cast<uint32_t>(I + 1))]);
	}
	return Mine[static_cast<size_t>(In - First)];
}

std::vector<std::string> IgniteInventory()
{
	// Every sentence Ignite's client draws beyond core's own copy (the lint), then the 150 shouts.
	std::vector<std::string> Out = {IgniteStaysHere, IgniteStartsIt, IgniteAllYours, IgniteWindowShut, IgniteBonusRule, IgniteNoneOpen, IgniteSeatsOpen, IgniteNoRegulars, IgniteClauseLine,
		IgniteNoNews, IgniteNoCash, IgniteNoRecords, IgniteNoLines, IgniteNothingComing, IgniteDepositHow, IgniteWithdrawHow, IgniteOnlyBank, IgniteNoBoard, IgniteCleanGame, IgniteNoGrades,
		IgniteDecisionsXp, IgniteNoShouts, "Your first deposit starts it.", "The 60 days are up.", "Top of the board.", "A final table here puts you on the board.",
		"Deposited {Amount}. It's on Ignite now.", "Requested. {Amount} is on its way to your bank."};
	for (const std::string& L : IgniteShouts())
	{
		Out.push_back(L);
	}
	return Out;
}

std::vector<std::string> IgniteLabels()
{
	// The words on buttons, signs, tabs and stickers (screened for trademarks and U1, not linted as sentences).
	std::vector<std::string> Out = {"PLAY", "REWARDS", "FLIP", "BOARDS", "WALLET", "WELCOME BONUS", "SUNDAY BLAZE", "TONIGHT \xC2\xB7 BANDSTAND", "UP NEXT \xC2\xB7 BANDSTAND", "WHO'S IN",
		"NEXT FLIP", "FLIP & RUN", "PRIZE COUNTER", "TONIGHT'S STARTS", "THE SPARK WHEEL", "HIGH SCORES", "YOU", "BUZZ", "What's lighting up the boardwalk", "DEPOSIT", "WITHDRAW",
		"RECEIPTS", "ON THE WAY", "ON ITS WAY BACK", "FUEL GAUGE", "RECORDS", "YOUR LAST RESULTS HERE", "EVENTS HERE", "WON HERE", "SEASON POINTS", "FINAL TABLES", "BEST FINISH",
		"BIGGEST CASH", "TITLES", "CASHES", "Play again", "Back to play", "HAND REVIEW", "SKILL XP", "No skill XP", "Decision accuracy", "ONLINE", "LATE", "REG", "SHOUT", "STARTING SOON",
		"BIG GUARANTEES TONIGHT", "FLIP & RUN \xC2\xB7 EVERY HOUR", "BOUNTIES & MYSTERY", "MY TICKETS", "CHEAP SHOTS", "TOMORROW MORNING", "Welcome bonus", "100% up to $100",
		"your first deposit starts it", "the 60 days are up", "all of it's yours", "RELEASED", "PENDING", "MATCHED", "UP TO", "PER $20", "NEXT", "EXPIRED", "GUARANTEED",
		"PRIZE POOL", "ENTRIES", "STARTS", "PLACES PAID", "STARTING STACK", "LATE REG", "LEVELS", "YOUR IGNITE BALANCE", "BUY-IN", "Overview", "Structure", "Payouts", "Players",
		"The night's lineup", "Nothing like it on the floor right now", "Locked", "YOU WON", "SEAT WON", "TONIGHT", "Everything", "Big hits", "Promos", "Shouts", "AFTERBURNER HOURS",
		"WAKE-UP", "DAYTIME", "PRIME TIME", "FEATURED", "ON NOW", "STARTS", "IN", "ALL IN", "CHECK", "CALL", "RAISE", "BET", "KO", "Your balance on Ignite", "Lands on Ignite",
		"The bank pays", "Bank after", "Leaves Ignite now", "Reaches your bank", "Expected", "next $5 in", "of rake", "events on the floor", "guaranteed", "OPEN THE WALLET",
		// The rest of what the pages draw around live figures (the sheet, the wallet, the results, REWARDS, BUZZ's stickers).
		"NEXT STARTS", "LATE REGISTRATION CLOSES IN", "REGISTRATION OPENS IN", "RUNNING", "FINAL TABLE", "FINISHED", "LEVEL", "Game", "Speed", "Starting stack",
		"Blinds", "Late registration", "Estimated finish", "NL Hold'em", "PL Omaha", "Free to play", "Free", "Prizes", "No regulars registered yet",
		"Regulars registered", "more regulars", "players expected", "a pool built by its entries", "structure", "minute levels", "places paid", "min cash",
		"in bounties", "knockout", "knockouts", "ticket held", "tickets held", "after", "Until", "Level 1", "chips", "Joined", "On Ignite since",
		"Ignite balance now", "Balance", "Bank", "Deposit", "Withdraw", "No score", "No way in yet", "No way out yet", "Minimum", "Short", "in the bank",
		"This sitting", "tournaments", "net", "Withdrawal of", "requested", "arriving", "to your bank", "YOUR TURN", "LIVE", "SERIES", "NEW", "RECORD",
		"PEOPLE", "BIG WIN", "PROMO", "LAUNCH", "OVERLAY", "pts", "pts to pass", "DAY", "OF", "100% of your first deposit, up to $100", "Expires", "left",
		"Expired", "of rake paid here so far", "YOUR GAME ON IGNITE", "TOURNAMENTS HERE", "RAKE THIS MONTH", "RAKE ALL TIME", "Open the Wallet", "up to $100",
		"still pending", "released", "Card", "lands at once", "Let's go"};
	for (const char* Chip : IgniteChipNames)
	{
		Out.push_back(Chip);
	}
	return Out;
}
} // namespace rlignite_detail
} // namespace ui
} // namespace ss
