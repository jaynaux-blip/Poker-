// The GearDrop monitors' pictures: Kast's studio or the life tracker on the 24", study on the 27". And the
// Embercrest's wall calendar by the window, drawn from the same career.
#include "ShortStack/UI/SecondScreen.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/StreamArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace ss
{
namespace ui
{
namespace agenda_detail
{
/**
 * The dates the screens show (next_segment_spec §1.3): every feature's marks from From to To (Session::Marks, through
 * AgendaOf), less the sponsors' appearance windows (an open window is the street's duty chip; daily ones would fill
 * every square).
 */
std::vector<life::Mark> Dates(const Session& S, double From, double To)
{
	std::vector<life::Mark> Out = AgendaOf(S, From, std::max(0.0, To - From) / net::MinutesPerDay).Marks;
	Out.erase(std::remove_if(Out.begin(), Out.end(), [](const life::Mark& M) { return M.Kind == sponsor::MarkAppearance; }), Out.end());
	return Out;
}

/** A date in a word, for a calendar square, and how much it matters (lower first): what's owed comes first. */
std::pair<int, std::string> Word(const life::Mark& M)
{
	if (M.Kind == life::LedgerBacking)
	{
		// The book's dates (stake::Marks): payments due, owed to the player, chop days, staked events, listings.
		if (M.Label.rfind("Chop day", 0) == 0)
		{
			return {2, "CHOP"};
		}
		if (M.Label.find(" owes you ") != std::string::npos)
		{
			return {6, "OWED"};
		}
		if (M.Label.rfind("Pieces sold", 0) == 0)
		{
			return {8, "SOLD"};
		}
		if (M.Label.find("putting you in") != std::string::npos)
		{
			return {7, "BACKED"};
		}
		return {0, "PAY"};
	}
	switch (M.Kind)
	{
	case sponsor::MarkCure: return {1, "CURE"};
	case sponsor::MarkReview: return {3, "REVIEW"};
	case sponsor::MarkRenewal: return {4, "RENEW"};
	case sponsor::MarkOfferEnds: return {5, "OFFER"};
	default: return {9, std::string()};
	}
}
} // namespace agenda_detail

namespace secondscreen
{
namespace secondscreen_detail
{
const float W = Width;
const float H = Height;
const Color Lime = Hex(0x53fc18);
const Color LiveRed = Hex(0xeb0400);

Color Alpha(const Color& C, float A)
{
	return Color{C.R, C.G, C.B, C.A * A};
}

void Backdrop(Canvas& C)
{
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, pal::Bg2, pal::Bg));
}

void Panel(Canvas& C, const Rect& R)
{
	C.FillRoundRect(R, 14.0f, Paint::Solid(pal::Panel));
	C.StrokeRoundRect(R, 14.0f, pal::Line, 1.5f);
}

/** "1:07" from minutes. */
std::string HoursMinutes(double Minutes)
{
	const int M = static_cast<int>(std::max(0.0, Minutes));
	const int Mm = M % 60;
	return std::to_string(M / 60) + (Mm < 10 ? ":0" : ":") + std::to_string(Mm);
}

std::string Signed(Chips Cents)
{
	return (Cents > 0 ? "+" : "") + Money(Cents);
}

/** A sparkline (area and line) of Values across R, scaled between Lo and Hi. */
void Graph(Canvas& C, const Rect& R, const std::vector<float>& Values, float Lo, float Hi, const Color& Col)
{
	if (Values.size() < 2)
	{
		C.StrokePolyline({{R.X, R.Y + R.H * 0.6f}, {R.X + R.W, R.Y + R.H * 0.6f}}, false, Alpha(Col, 0.5f), 2.0f);
		return;
	}
	const float Span = std::max(Hi - Lo, 1e-3f);
	std::vector<Vec2> Pts;
	Pts.reserve(Values.size());
	for (size_t I = 0; I < Values.size(); ++I)
	{
		const float X = R.X + R.W * static_cast<float>(I) / static_cast<float>(Values.size() - 1);
		const float Y = R.Y + R.H * (1.0f - (Values[I] - Lo) / Span);
		Pts.push_back({X, Y});
	}
	// The area under it, a strip of quads (each convex), fading downward.
	const float Bottom = R.Y + R.H;
	for (size_t I = 0; I + 1 < Pts.size(); ++I)
	{
		C.FillPolygon({Pts[I], Pts[I + 1], {Pts[I + 1].X, Bottom}, {Pts[I].X, Bottom}},
		              Paint::Linear({0.0f, R.Y}, {0.0f, Bottom}, Alpha(Col, 0.32f), Alpha(Col, 0.02f)));
	}
	C.StrokePolyline(Pts, false, Col, 3.0f, true);
	C.FillCircle(Pts.back().X, Pts.back().Y, 6.0f, Paint::Solid(Col));
	C.FillCircle(Pts.back().X, Pts.back().Y, 12.0f, Paint::Solid(Alpha(Col, 0.25f)));
}

// ------------------------------------------------------------------ the life tracker

void Stat(Canvas& C, const Rect& R, const std::string& Title, const std::string& Big, const Color& BigCol, const std::string& Sub)
{
	Panel(C, R);
	C.Text(Title, R.X + 22.0f, R.Y + 36.0f, Ts(15.0f, 800, pal::Muted));
	C.Text(Big, R.X + 22.0f, R.Y + 84.0f, Ts(36.0f, 800, BigCol, Align::Left, Baseline::Alphabetic, false, R.W - 44.0f));
	C.Text(Sub, R.X + 22.0f, R.Y + 118.0f, Ts(16.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 44.0f));
}

void Tracker(Canvas& C, const Session& S, double Now)
{
	Backdrop(C);
	const life::State& L = S.Life;
	const double World = S.WorldMinutes();
	C.Text("BANKROLL", 60.0f, 82.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(Money(S.BankrollCents), 60.0f, 160.0f, Ts(72.0f, 800, pal::Ink));
	Chips Day = 0;
	for (const life::LedgerEntry& E : L.Ledger)
	{
		if (E.At < World - 1440.0)
		{
			break;
		}
		Day += E.Amount;
	}
	if (Day != 0)
	{
		C.Text(Signed(Day) + " in the last 24 hours", 64.0f, 200.0f, Ts(22.0f, 700, Day > 0 ? pal::Green : pal::Red));
	}
	C.Text(ClockString(S.ClockMinutes()), W - 60.0f, 82.0f, Ts(22.0f, 700, pal::Ink, Align::Right));
	C.Text(S.HeroName, W - 60.0f, 112.0f, Ts(16.0f, 600, pal::Muted, Align::Right));

	// The bankroll's story, from the ledger (newest first): walk back from today's balance.
	const Rect G{60.0f, 236.0f, W - 120.0f, 300.0f};
	Panel(C, {G.X - 20.0f, G.Y - 20.0f, G.W + 40.0f, G.H + 40.0f});
	std::vector<float> Curve;
	const size_t N = std::min<size_t>(L.Ledger.size(), 40);
	Chips B = S.BankrollCents;
	Curve.push_back(static_cast<float>(B));
	for (size_t I = 0; I < N; ++I)
	{
		B -= L.Ledger[I].Amount;
		Curve.push_back(static_cast<float>(B));
	}
	std::reverse(Curve.begin(), Curve.end());
	float Lo = *std::min_element(Curve.begin(), Curve.end());
	float Hi = *std::max_element(Curve.begin(), Curve.end());
	const float Pad = std::max((Hi - Lo) * 0.15f, 500.0f);
	Lo = std::max(0.0f, Lo - Pad);
	Hi += Pad;
	for (int K = 1; K < 4; ++K)
	{
		const float Y = G.Y + G.H * static_cast<float>(K) / 4.0f;
		C.StrokePolyline({{G.X, Y}, {G.X + G.W, Y}}, false, Alpha(pal::Line, 0.7f), 1.0f);
		C.Text(Money(static_cast<Chips>(Hi - (Hi - Lo) * static_cast<float>(K) / 4.0f)), G.X + G.W, Y - 6.0f, Ts(13.0f, 600, pal::Dim, Align::Right));
	}
	const bool Up = Curve.back() >= Curve.front();
	Graph(C, G, Curve, Lo, Hi, Up ? pal::Accent : pal::Red);
	if (Curve.size() < 3)
	{
		C.Text("The story starts tonight.", G.X + G.W / 2.0f, G.Y + G.H / 2.0f - 30.0f, Ts(22.0f, 600, pal::Muted, Align::Center));
	}

	// Rent, Dee's game and the Embercrest.
	const float Ty = 586.0f;
	const float Tw = (W - 120.0f - 40.0f) / 3.0f;
	{
		std::string Big = Money(L.RentDueCents);
		std::string Sub;
		Color Col = pal::Ink;
		const double Left = L.RentDeadline - World;
		switch (L.RentStage)
		{
		case life::Rent::Paid: Big = "PAID"; Col = pal::Green; Sub = "Next: " + Money(L.RentDueCents) + " in " + std::to_string(static_cast<int>(std::max(0.0, Left) / 1440.0)) + " days"; break;
		case life::Rent::FinalNotice: Col = pal::Red; Sub = "FINAL NOTICE \xC2\xB7 " + HoursMinutes(Left) + " left"; break;
		case life::Rent::Evicted: Big = "EVICTED"; Col = pal::Red; Sub = "A key back: " + Money(L.RentDueCents); break;
		default:
			Col = Left < 1440.0 ? pal::Orange : pal::Ink;
			Sub = Left > 1440.0 ? "Due in " + std::to_string(static_cast<int>(Left / 1440.0)) + "d " + std::to_string(static_cast<int>(std::fmod(Left, 1440.0) / 60.0)) + "h"
			                    : "Due in " + HoursMinutes(std::max(0.0, Left));
			break;
		}
		Stat(C, {60.0f, Ty, Tw, 140.0f}, "RENT", Big, Col, Sub);
	}
	Stat(C, {60.0f + Tw + 20.0f, Ty, Tw, 140.0f}, "DEE'S GAME", L.BackRoomNights > 0 ? Signed(L.BackRoomNetCents) : std::string("\xE2\x80\x94"),
	     L.BackRoomNetCents > 0 ? pal::Green : (L.BackRoomNetCents < 0 ? pal::Red : pal::Ink),
	     L.BackRoomNights > 0 ? std::to_string(L.BackRoomNights) + (L.BackRoomNights == 1 ? " night in the back room" : " nights in the back room") : std::string("Laundromat, back room. Ask Dee."));
	// The Embercrest: the player's entry there, else the next event the bus can still make; their record in the corner.
	{
		namespace rl = rlnet_detail;
		const Rect Er{60.0f + 2.0f * (Tw + 20.0f), Ty, Tw, 140.0f};
		std::string Big = "\xE2\x80\x94";
		std::string Sub = "6-max freezeouts every day.";
		Color BigCol = pal::Ink;
		const life::LiveEntry* Mine = live::ActiveEntry(L);
		const live::Occurrence Entered = Mine ? live::FindOccurrence(Mine->Id) : live::Occurrence();
		const std::vector<live::Occurrence> Next = rl::LiveAhead(S, World, 48.0);
		if (Mine && Entered.Valid())
		{
			const rl::LiveCard Card = rl::LiveCardOf(S, Entered, World);
			Big = "You're in: " + std::string(Entered.T->Short);
			BigCol = Hex(rl::EcEmberHot);
			Sub = Card.When + " " + net::TimeLabel(Entered.Start) + " \xC2\xB7 " + rl::NetMoney(Mine->PaidCents) + " paid" + (Mine->ArrivedAt > 0.0 ? " \xC2\xB7 the night's under way" : "");
		}
		else if (!Next.empty())
		{
			const live::Occurrence& O = Next.front();
			const rl::LiveCard Card = rl::LiveCardOf(S, O, World);
			Big = O.T->Short;
			const std::string Price = rl::NetMoney(O.T->BuyInCents);
			Sub = Card.Phase == rl::LiveCard::LateReg ? "Late reg till " + net::TimeLabel(O.LateRegEnds) + " \xC2\xB7 " + Price
				: Card.When + " " + net::TimeLabel(O.Start) + " \xC2\xB7 " + Price + (World < O.DeskOpens ? " \xC2\xB7 desk opens " + net::TimeLabel(O.DeskOpens) : " \xC2\xB7 the desk is open");
			if (S.BankrollCents < Card.Cost)
			{
				// Out of reach: the bankroll doesn't cover the buy-in and the bus.
				BigCol = pal::Muted;
				Sub = Card.When + " " + net::TimeLabel(O.Start) + " \xC2\xB7 " + Price + " \xC2\xB7 " + rl::NetMoney(Card.Cost - S.BankrollCents) + " short with the bus";
			}
		}
		Stat(C, Er, "THE EMBERCREST", Big, BigCol, Sub);
		if (L.LiveEvents > 0)
		{
			C.Text(std::to_string(L.LiveEvents) + " played \xC2\xB7 " + std::to_string(L.LiveCashes) + " cashed" + (L.LiveBestPlace > 0 ? " \xC2\xB7 best " + Ordinal(L.LiveBestPlace) : std::string()),
			       Er.X + Er.W - 22.0f, Er.Y + 36.0f, Ts(14.0f, 600, L.LiveBestPlace == 1 ? pal::Gold : pal::Muted, Align::Right));
		}
	}

	// The latest entries, and what's coming up beside them (§1.3: chop days, payments due, a sponsor's review).
	const std::vector<life::Mark> Coming = agenda_detail::Dates(S, World, World + 14.0 * net::MinutesPerDay);
	const float Right = Coming.empty() ? W - 60.0f : W * 0.5f - 30.0f;
	float Y = 776.0f;
	C.Text("LATELY", 60.0f, Y, Ts(15.0f, 800, pal::Muted));
	for (size_t I = 0; I < std::min<size_t>(L.Ledger.size(), 3); ++I)
	{
		const life::LedgerEntry& E = L.Ledger[I];
		const float Ly = Y + 34.0f + static_cast<float>(I) * 30.0f;
		const float Aw = C.Text(Signed(E.Amount), Right, Ly, Ts(18.0f, 700, E.Amount >= 0 ? pal::Green : pal::Red, Align::Right));
		C.Text(E.Label, 60.0f, Ly, Ts(18.0f, 500, pal::Ink, Align::Left, Baseline::Alphabetic, false, Right - 60.0f - Aw - 24.0f));
	}
	if (!Coming.empty())
	{
		const float Cx = W * 0.5f + 30.0f;
		C.Text("COMING UP", Cx, Y, Ts(15.0f, 800, pal::Muted));
		for (size_t I = 0; I < std::min<size_t>(Coming.size(), 3); ++I)
		{
			const life::Mark& M = Coming[I];
			const float Ly = Y + 34.0f + static_cast<float>(I) * 30.0f;
			const int On = net::DayOf(M.At);
			const int Today = net::DayOf(World);
			const std::string When = On == Today ? std::string("Today") : On == Today + 1 ? std::string("Tomorrow") : std::string(net::WeekdayName(On, true));
			const float Ww = C.Text(When, Cx, Ly, Ts(18.0f, 700, agenda_detail::Word(M).first <= 1 ? pal::Orange : pal::Accent));
			C.Text(M.Label, Cx + std::max(Ww, 110.0f) + 14.0f, Ly, Ts(18.0f, 500, pal::Ink, Align::Left, Baseline::Alphabetic, false, W - 60.0f - (Cx + std::max(Ww, 110.0f) + 14.0f)));
		}
	}
	(void)Now;
}

// ------------------------------------------------------------------ Kast studio

void Studio(Canvas& C, const Session& S, double Now)
{
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Solid(Hex(0x0e0e10)));
	const kast::Stream& St = S.Stream;
	const double World = S.WorldMinutes();
	// Header: LIVE, uptime, viewers, followers.
	const float Pulse = 0.65f + 0.35f * static_cast<float>(std::sin(Now * 3.0));
	C.FillRoundRect({40.0f, 28.0f, 96.0f, 40.0f}, 8.0f, Paint::Solid(Alpha(LiveRed, Pulse)));
	C.Text("LIVE", 88.0f, 56.0f, Ts(20.0f, 900, Hex(0xffffff), Align::Center));
	C.Text(HoursMinutes(St.Uptime(World)), 156.0f, 57.0f, Ts(22.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	const std::string Viewers = std::to_string(static_cast<int>(std::round(St.Viewers)));
	C.Text(Viewers + " watching", 330.0f, 57.0f, Ts(22.0f, 800, Lime));
	C.Text(std::to_string(S.Channel.Followers) + " followers", 560.0f, 57.0f, Ts(20.0f, 600, pal::Muted));
	C.Text("KAST STUDIO", W - 40.0f, 57.0f, Ts(18.0f, 900, Hex(0x9b8cff), Align::Right));

	// The program: the facecam as it goes out, with the overlay's name strip.
	const Rect Pg{40.0f, 96.0f, 960.0f, 540.0f};
	C.Text("PROGRAM", Pg.X, Pg.Y - 8.0f, Ts(13.0f, 800, LiveRed));
	streamart::Cam Look;
	Look.Gear = S.GearFx();
	Look.Headphones = S.Owns("headphones");
	Look.Face = St.Face;
	Look.FaceAge = Now - St.FaceAt;
	Look.Talking = Now - St.ThankAt < 3.0;
	Look.Time = Now;
	Look.Live = St.Live;
	Look.Leds = S.RoomGlow(Now);
	C.Save();
	C.PushClip(Pg);
	streamart::Facecam(C, Pg, Look);
	C.PopClip();
	C.Restore();
	C.StrokeRoundRect(Pg, 4.0f, LiveRed, 3.0f);
	C.FillRect({Pg.X, Pg.Y + Pg.H - 56.0f, Pg.W, 56.0f}, Paint::Linear({0.0f, Pg.Y + Pg.H - 56.0f}, {0.0f, Pg.Y + Pg.H}, Alpha(Hex(0x000000), 0.0f), Alpha(Hex(0x000000), 0.75f)));
	C.Text(S.HeroName, Pg.X + 24.0f, Pg.Y + Pg.H - 18.0f, Ts(24.0f, 900, Hex(0xffffff)));
	C.Text("SHORT STACK \xC2\xB7 grinding the night", Pg.X + 28.0f + C.Measure(S.HeroName, 24.0f, 900), Pg.Y + Pg.H - 18.0f, Ts(18.0f, 600, Alpha(Hex(0xffffff), 0.7f)));
	// The latest alert, over the program's top.
	if (!St.Alerts.empty())
	{
		const kast::Alert& A = St.Alerts.front();
		const Rect Ar{Pg.X + Pg.W / 2.0f - 260.0f, Pg.Y + 20.0f, 520.0f, 64.0f};
		C.FillRoundRect(Ar, 12.0f, Paint::Solid(Alpha(Hex(0x1b1530), 0.92f)));
		C.StrokeRoundRect(Ar, 12.0f, Hex(0x9b8cff), 2.0f);
		C.Text(A.Who, Ar.X + Ar.W / 2.0f, Ar.Y + 30.0f, Ts(22.0f, 900, Lime, Align::Center, Baseline::Alphabetic, false, Ar.W - 30.0f));
		C.Text(A.Text, Ar.X + Ar.W / 2.0f, Ar.Y + 54.0f, Ts(15.0f, 500, pal::Ink, Align::Center, Baseline::Alphabetic, false, Ar.W - 30.0f));
	}

	// Viewers over the stream.
	const Rect Gr{40.0f, 676.0f, 960.0f, 170.0f};
	C.Text("VIEWERS", Gr.X, Gr.Y - 10.0f, Ts(13.0f, 800, pal::Muted));
	C.Text("peak " + std::to_string(St.Peak), Gr.X + Gr.W, Gr.Y - 10.0f, Ts(13.0f, 700, pal::Muted, Align::Right));
	std::vector<float> V(St.Graph.end() - std::min<ptrdiff_t>(static_cast<ptrdiff_t>(St.Graph.size()), 90), St.Graph.end());
	float Hi = 4.0f;
	for (float X : V)
	{
		Hi = std::max(Hi, X * 1.2f);
	}
	Graph(C, Gr, V, 0.0f, Hi, Lime);

	// Chat, newest at the bottom.
	const Rect Ch{1040.0f, 96.0f, W - 1080.0f, 750.0f};
	C.FillRoundRect(Ch, 10.0f, Paint::Solid(Hex(0x18181b)));
	C.Text("STREAM CHAT", Ch.X + 20.0f, Ch.Y + 36.0f, Ts(15.0f, 800, pal::Muted));
	C.PushClip({Ch.X, Ch.Y + 50.0f, Ch.W, Ch.H - 60.0f});
	float Y = Ch.Y + Ch.H - 22.0f;
	for (size_t I = St.Chat.size(); I-- > 0 && Y > Ch.Y + 60.0f;)
	{
		const kast::ChatMsg& M = St.Chat[I];
		if (M.Deleted)
		{
			continue;
		}
		const bool Named = M.Kind == kast::LineKind::Chat || M.Kind == kast::LineKind::Streamer || M.Kind == kast::LineKind::Question || M.Kind == kast::LineKind::Tip;
		float X = Ch.X + 20.0f;
		if (Named)
		{
			X += C.Text(M.Who + ":", X, Y, Ts(17.0f, 800, Hex(M.NameColor))) + 8.0f;
		}
		C.Text(M.Text, X, Y, Ts(17.0f, 500, Named ? pal::Ink : Hex(0xa78bfa), Align::Left, Baseline::Alphabetic, false, Ch.X + Ch.W - 20.0f - X));
		Y -= 30.0f;
	}
	C.PopClip();
}

// ------------------------------------------------------------------ study

struct Hand
{
	int Hi;
	int Lo;
	bool Suited;
};

/** The Chen formula: a quick score for a starting hand (ranks 2..14). */
double Chen(const Hand& Hd)
{
	auto Val = [](int R) { return R == 14 ? 10.0 : R == 13 ? 8.0 : R == 12 ? 7.0 : R == 11 ? 6.0 : R / 2.0; };
	double Sc = Val(Hd.Hi);
	if (Hd.Hi == Hd.Lo)
	{
		return std::max(5.0, Sc * 2.0);
	}
	if (Hd.Suited)
	{
		Sc += 2.0;
	}
	const int Gap = Hd.Hi - Hd.Lo - 1;
	Sc -= Gap == 0 ? 0.0 : Gap == 1 ? 1.0 : Gap == 2 ? 2.0 : Gap == 3 ? 4.0 : 5.0;
	if (Gap <= 1 && Hd.Hi < 12)
	{
		Sc += 1.0;
	}
	return std::ceil(Sc);
}

struct Position
{
	const char* Name;
	double Open; // share of hands opened
};
const Position Positions[5] = {{"UTG", 0.15}, {"HJ", 0.19}, {"CO", 0.27}, {"BTN", 0.43}, {"SB", 0.36}};

/** 0 fold, 1 mixed (the edge of the range), 2 raise; [row][col], row/col 0 = ace. */
std::array<std::array<int, 13>, 13> Chart(int Pos)
{
	struct Entry
	{
		int Row, Col;
		double Score;
		int Combos;
	};
	std::vector<Entry> All;
	for (int R = 0; R < 13; ++R)
	{
		for (int Cc = 0; Cc < 13; ++Cc)
		{
			const int A = 14 - R;
			const int B = 14 - Cc;
			const Hand Hd{std::max(A, B), std::min(A, B), Cc > R};
			const int Combos = R == Cc ? 6 : (Cc > R ? 4 : 12);
			// Ties: pairs, then suited, then the higher kicker.
			All.push_back({R, Cc, Chen(Hd) + (R == Cc ? 0.3 : 0.0) + (Hd.Suited ? 0.1 : 0.0) + Hd.Lo * 0.01, Combos});
		}
	}
	std::stable_sort(All.begin(), All.end(), [](const Entry& X, const Entry& Y) { return X.Score > Y.Score; });
	std::array<std::array<int, 13>, 13> Out{};
	const double Goal = Positions[Pos].Open * 1326.0;
	double Sum = 0.0;
	for (const Entry& E : All)
	{
		if (Sum >= Goal * 1.08)
		{
			break;
		}
		Out[E.Row][E.Col] = Sum < Goal * 0.94 ? 2 : 1;
		Sum += E.Combos;
	}
	return Out;
}

const char* RankChar(int Index)
{
	static const char* Ranks[13] = {"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
	return Ranks[Index];
}

std::string TellPhrase(const std::string& Tell)
{
	static const std::map<std::string, std::string> Phrases = {
		{"ChipGlance", "the glance at the chips"}, {"BrowFlash", "the eyebrow flash"}, {"Swallow", "the dry swallow"}, {"LipPress", "the pressed lips"},
		{"Freeze", "going still"}, {"StareDown", "the stare"}, {"LookAway", "looking away"}, {"Tremble", "the shaking hands"},
		{"NeckTouch", "the hand to the neck"}, {"FalseSmile", "the mouth-only smile"}, {"RealSmile", "the real smile"}, {"BlinkBurst", "the blinking"},
		{"Sigh", "the big sigh"}, {"Recheck", "checking the cards again"}, {"PupilFlare", "the wide pupils"}, {"ChipReach", "reaching for chips"},
	};
	const auto It = Phrases.find(Tell);
	return It == Phrases.end() ? Tell : It->second;
}

void Study(Canvas& C, const Session& S, double Now)
{
	Backdrop(C);
	// The chart cycles through the positions; each change sweeps across the grid from the top left.
	const double Period = 9.0;
	const int Pos = static_cast<int>(std::fmod(Now / Period, 5.0));
	const int Prev = (Pos + 4) % 5;
	const double Since = std::fmod(Now, Period);
	static std::array<std::array<std::array<int, 13>, 13>, 5> Charts;
	static bool bCharts = false;
	if (!bCharts)
	{
		for (int P = 0; P < 5; ++P)
		{
			Charts[P] = Chart(P);
		}
		bCharts = true;
	}
	C.Text("PREFLOP \xC2\xB7 OPEN RAISE", 60.0f, 76.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(std::string(Positions[Pos].Name) + " \xC2\xB7 " + std::to_string(static_cast<int>(Positions[Pos].Open * 100.0 + 0.5)) + "% of hands", 60.0f, 118.0f, Ts(32.0f, 800, pal::Ink));
	for (int P = 0; P < 5; ++P)
	{
		const float Px = 470.0f + static_cast<float>(P) * 80.0f;
		const bool On = P == Pos;
		C.FillRoundRect({Px, 90.0f, 70.0f, 34.0f}, 17.0f, Paint::Solid(On ? pal::Accent : pal::Panel2));
		C.Text(Positions[P].Name, Px + 35.0f, 113.0f, Ts(15.0f, 800, On ? Hex(0x04201c) : pal::Muted, Align::Center));
	}
	const float Cell = 52.0f;
	const float Gx = 60.0f;
	const float Gy = 150.0f;
	const Color Fold = Hex(0x1a2436);
	const Color Mixed = Hex(0xf28a3a);
	const Color Raise = Hex(0xe5484d);
	auto Shade = [&](int State, bool Pair) {
		const Color Base = State == 2 ? Raise : State == 1 ? Mixed : Fold;
		return Pair ? Mix(Base, Hex(0xffffff), State ? 0.08f : 0.03f) : Base;
	};
	for (int R = 0; R < 13; ++R)
	{
		for (int Cc = 0; Cc < 13; ++Cc)
		{
			const float T = static_cast<float>(Clamp01(Since * 3.0 - (R + Cc) * 0.06));
			const Color Col = Mix(Shade(Charts[Prev][R][Cc], R == Cc), Shade(Charts[Pos][R][Cc], R == Cc), T);
			const Rect Rc{Gx + static_cast<float>(Cc) * Cell, Gy + static_cast<float>(R) * Cell, Cell - 3.0f, Cell - 3.0f};
			C.FillRoundRect(Rc, 6.0f, Paint::Solid(Col));
			const int Hi = std::min(R, Cc);
			const int Lo = std::max(R, Cc);
			const std::string Label = std::string(RankChar(Hi)) + RankChar(Lo) + (R == Cc ? "" : (Cc > R ? "s" : "o"));
			C.Text(Label, Rc.X + Rc.W / 2.0f, Rc.Y + Rc.H / 2.0f + 6.0f, Ts(15.0f, 700, Charts[Pos][R][Cc] ? Hex(0xffffff) : pal::Muted, Align::Center));
		}
	}
	const float Ly = Gy + 13.0f * Cell + 30.0f;
	C.FillRoundRect({Gx, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Raise));
	C.Text("raise", Gx + 30.0f, Ly, Ts(16.0f, 600, pal::Muted));
	C.FillRoundRect({Gx + 120.0f, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Mixed));
	C.Text("sometimes", Gx + 150.0f, Ly, Ts(16.0f, 600, pal::Muted));
	C.FillRoundRect({Gx + 290.0f, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Fold));
	C.Text("fold", Gx + 320.0f, Ly, Ts(16.0f, 600, pal::Muted));

	// Reads: the tells the player has seen confirmed at showdown.
	const Rect Rd{800.0f, 60.0f, W - 860.0f, 560.0f};
	Panel(C, Rd);
	C.Text("READS", Rd.X + 28.0f, Rd.Y + 48.0f, Ts(18.0f, 800, pal::Gold));
	C.Text("from the back room", Rd.X + 110.0f, Rd.Y + 48.0f, Ts(16.0f, 500, pal::Muted));
	std::vector<std::pair<int, std::string>> Known;
	for (const auto& R : S.Life.Reads)
	{
		Known.push_back({R.second, R.first});
	}
	std::stable_sort(Known.begin(), Known.end(), [](const auto& X, const auto& Y) { return X.first > Y.first; });
	if (Known.empty())
	{
		C.Text("Nothing yet. Watch them at Dee's table:", Rd.X + 28.0f, Rd.Y + 110.0f, Ts(20.0f, 500, pal::Ink));
		C.Text("what they do with a big hand, and with nothing.", Rd.X + 28.0f, Rd.Y + 142.0f, Ts(20.0f, 500, pal::Muted));
	}
	float Y = Rd.Y + 108.0f;
	for (size_t I = 0; I < Known.size() && I < 8; ++I)
	{
		const std::string& Key = Known[I].second;
		const size_t Slash = Key.find('/');
		const std::string Who = Slash == std::string::npos ? Key : Key.substr(0, Slash);
		const std::string Tell = Slash == std::string::npos ? std::string() : TellPhrase(Key.substr(Slash + 1));
		const bool Learned = Known[I].first >= 2;
		C.FillCircle(Rd.X + 36.0f, Y - 7.0f, 6.0f, Paint::Solid(Learned ? pal::Accent : pal::Dim));
		const float Nx = Rd.X + 54.0f;
		const float Nw = C.Text(Who, Nx, Y, Ts(21.0f, 800, pal::Ink));
		C.Text(Tell, Nx + Nw + 12.0f, Y, Ts(20.0f, 500, Learned ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, false, Rd.X + Rd.W - 120.0f - (Nx + Nw + 12.0f)));
		C.Text(Learned ? "read" : "seen " + std::to_string(Known[I].first) + "x", Rd.X + Rd.W - 28.0f, Y, Ts(16.0f, 700, Learned ? pal::Accent : pal::Dim, Align::Right));
		Y += 54.0f;
	}

	// Tonight at the tables.
	const Rect Tn{800.0f, 650.0f, W - 860.0f, 190.0f};
	Panel(C, Tn);
	C.Text("TONIGHT", Tn.X + 28.0f, Tn.Y + 44.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(std::to_string(S.HandsPlayed), Tn.X + 28.0f, Tn.Y + 120.0f, Ts(56.0f, 800, pal::Ink));
	C.Text("hands played", Tn.X + 28.0f, Tn.Y + 160.0f, Ts(18.0f, 500, pal::Muted));
	C.Text(ClockString(S.ClockMinutes()), Tn.X + Tn.W - 28.0f, Tn.Y + 120.0f, Ts(40.0f, 700, pal::Ink, Align::Right));
	C.Text("and the rain hasn't stopped", Tn.X + Tn.W - 28.0f, Tn.Y + 160.0f, Ts(18.0f, 500, pal::Muted, Align::Right));
}
} // namespace secondscreen_detail

using namespace secondscreen_detail;

void Draw(Canvas& C, const Session& S, int Which, double Now)
{
	if (Which == 0)
	{
		if (S.Streaming())
		{
			Studio(C, S, Now);
		}
		else
		{
			Tracker(C, S, Now);
		}
	}
	else
	{
		Study(C, S, Now);
	}
}
} // namespace secondscreen

// ------------------------------------------------------------------ the wall calendar

namespace wallcalendar
{
namespace wallcalendar_detail
{
namespace rl = rlnet_detail;
const float W = Width;
const float H = Height;
const Color Print = Hex(0x1b2238);  // the calendar's own ink
const Color Pen = Hex(0x26408f);    // the player's ballpoint (days crossed off)
const Color Marker = Hex(0xc8231c); // the player's red marker (the rent, their entry)
const float GridX = 36.0f;
const float GridW = W - 72.0f;
const float Cell = GridW / 7.0f;

float F(int V)
{
	return static_cast<float>(V);
}

/** A loose marker ring, a little past closed, the way a hand draws one. */
void Ring(Canvas& C, float Cx, float Cy, float Rx, float Ry, const Color& Col, float Line)
{
	std::vector<Vec2> P;
	for (int K = 0; K <= 44; ++K)
	{
		const float A = -2.3f + F(K) / 44.0f * (2.0f * Pi + 0.45f);
		const float Wobble = 1.0f + 0.05f * std::sin(A * 3.0f + 0.8f);
		P.push_back({Cx + std::cos(A) * Rx * Wobble, Cy + std::sin(A) * Ry * Wobble});
	}
	C.StrokePolyline(P, false, Col, Line, true);
}

/** The month's squares with something written in them: one word a day in the player's ballpoint, the one that matters most. */
std::map<int, std::string> MonthMarks(const Session& S, int Today)
{
	int Year = 0;
	int Month = 0;
	int Date = 0;
	world::CivilDate(Today, Year, Month, Date);
	const int First = Today - (Date - 1);
	std::map<int, std::pair<int, std::string>> Best;
	for (const life::Mark& M : agenda_detail::Dates(S, static_cast<double>(First) * net::MinutesPerDay, static_cast<double>(First + 32) * net::MinutesPerDay))
	{
		const std::pair<int, std::string> Said = agenda_detail::Word(M);
		const int D = net::DayOf(M.At);
		const auto It = Best.find(D);
		if (!Said.second.empty() && (It == Best.end() || Said.first < It->second.first))
		{
			Best[D] = Said;
		}
	}
	std::map<int, std::string> Out;
	for (const auto& B : Best)
	{
		Out[B.first] = B.second.second;
	}
	return Out;
}

/** An event the bus can't make any more (late registration closes before it gets there), or one the room called off. */
bool Gone(const Session& S, const live::Occurrence& O, double World)
{
	return O.LateRegEnds < World + live::TravelMinutes || S.Living().CancellationOf(O.Id) != nullptr;
}

/** The event lines for one day: its time, its name, its price; the player's entry ringed, what's gone by struck out. */
float DayBlock(Canvas& C, const Session& S, int Day, const std::string& Title, float Y, double World)
{
	const life::State& L = S.Life;
	rl::NetSpaced(C, Title, GridX, Y, 17.0f, 900, Hex(rl::EcEmber), 2.2f);
	C.FillRect({GridX, Y + 9.0f, GridW, 2.0f}, Hex(rl::EcEmber, 0.6f));
	Y += 40.0f;
	for (const live::Occurrence& O : live::Occurrences(Day))
	{
		const life::LiveEntry* E = live::EntryFor(L, O.Id);
		const bool Struck = !E && Gone(S, O, World);
		const Color Ink = Struck ? Hex(0x8e8a80) : Print;
		const float Tw = C.Text(rl::LiveTime(O.Start), GridX, Y, Ts(28.0f, 900, Ink));
		C.Text(O.T->Short, GridX + 150.0f, Y, Ts(28.0f, 700, Ink, Align::Left, Baseline::Alphabetic, false, E ? GridW - 150.0f - 164.0f : GridW - 150.0f - 92.0f));
		C.Text(rl::NetMoney(O.T->BuyInCents), GridX + GridW, Y, Ts(28.0f, 900, Struck ? Ink : Hex(rl::EcEmberDeep), Align::Right));
		if (Struck)
		{
			C.StrokePolyline({{GridX - 4.0f, Y - 10.0f}, {GridX + GridW + 4.0f, Y - 8.0f}}, false, Rgba(38, 64, 143, 0.7f), 2.5f, true);
		}
		if (E)
		{
			// The player's: ringed in marker, with how it went once it's played.
			Ring(C, GridX + Tw * 0.5f, Y - 10.0f, Tw * 0.5f + 14.0f, 21.0f, Marker, 3.5f);
			const std::string Note = E->State == life::LiveEntry::Registered ? std::string("I'M IN") : E->State == life::LiveEntry::Refunded ? std::string("REFUNDED")
				: Ordinal(E->Place) + (E->PrizeCents > 0 ? " +" + rl::NetMoney(E->PrizeCents) : std::string());
			C.Text(Note, GridX + GridW - 82.0f, Y - 7.0f, Ts(15.0f, 900, Marker, Align::Right));
		}
		Y += 38.0f;
	}
	return Y;
}
} // namespace wallcalendar_detail

using namespace wallcalendar_detail;

std::string Key(const Session& S)
{
	const double World = S.WorldMinutes();
	const int Today = net::DayOf(World);
	const life::State& L = S.Life;
	std::string K = std::to_string(Today) + "|" + std::to_string(static_cast<int>(L.RentStage)) + "|" + std::to_string(net::DayOf(L.RentDeadline - 1.0)) + "|" + std::to_string(L.RentDueCents);
	// Today's and tomorrow's events: struck out once the bus can't make late registration any more, or called off.
	for (int Day = Today; Day <= Today + 1; ++Day)
	{
		for (const live::Occurrence& O : live::Occurrences(Day))
		{
			K += Gone(S, O, World) ? "x" : "o";
		}
	}
	// The player's entries mark their days.
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		K += "|" + E.Id + ":" + std::to_string(E.State) + ":" + std::to_string(E.Place) + ":" + std::to_string(E.PrizeCents);
	}
	// What else is written on the month: chop days, payments due, a sponsor's review (§1.3).
	for (const auto& Mark : MonthMarks(S, Today))
	{
		K += "|m" + std::to_string(Mark.first) + Mark.second;
	}
	return K;
}

void Draw(Canvas& C, const Session& S)
{
	const double World = S.WorldMinutes();
	const int Today = net::DayOf(World);
	const life::State& L = S.Life;
	// The sheet, and the wire binding it hangs from.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W * 0.25f, H}, Rgba(246, 242, 233, 1.0f), Rgba(232, 226, 212, 1.0f)));
	C.FillRect({0.0f, H - 10.0f, W, 10.0f}, Paint::Linear({0.0f, H - 10.0f}, {0.0f, H}, Rgba(0, 0, 0, 0.0f), Rgba(0, 0, 0, 0.12f)));
	// The wire binding: a loop through each punched hole, the hanger bent up in the middle where the nail takes it.
	const Color Wire = Hex(0x2b2d33);
	for (float X = 22.0f; X < W - 14.0f; X += 14.0f)
	{
		if (std::fabs(X - W * 0.5f) < 22.0f)
		{
			continue;
		}
		C.FillCircle(X, 17.0f, 2.4f, Rgba(20, 20, 24, 0.85f));
		C.StrokeArc(X, 11.0f, 3.6f, Pi, 2.0f * Pi, Wire, 1.6f);
		C.StrokePolyline({{X - 3.6f, 11.0f}, {X - 3.6f, 16.0f}}, false, Wire, 1.6f);
		C.StrokePolyline({{X + 3.6f, 11.0f}, {X + 3.6f, 16.0f}}, false, Wire, 1.6f);
	}
	C.StrokePolyline({{W * 0.5f - 20.0f, 15.0f}, {W * 0.5f - 6.0f, 3.0f}, {W * 0.5f + 6.0f, 3.0f}, {W * 0.5f + 20.0f, 15.0f}}, false, Wire, 2.2f, true);

	// The room's header: the mark over the ranges, its name, what it runs.
	const Rect Band{0.0f, 24.0f, W, 128.0f};
	C.FillRect(Band, Paint::Linear({0.0f, Band.Y}, {0.0f, Band.Y + Band.H}, Hex(rl::EcMidnight), Hex(rl::EcNavy)));
	C.PushClip(Band);
	C.FillEllipse(84.0f, 80.0f, 120.0f, 80.0f, Paint::Radial({84.0f, 80.0f}, 0.0f, {84.0f, 80.0f}, 120.0f, Hex(rl::EcEmberHot, 0.28f), -1.0f, Color(), Hex(rl::EcEmberHot, 0.0f)));
	std::vector<Vec2> Ridge;
	for (float X = 0.0f; X <= W + 20.0f; X += 20.0f)
	{
		Ridge.push_back({X, Band.Y + Band.H - 22.0f + 7.0f * std::sin(X * 0.017f) + 4.0f * std::sin(X * 0.051f + 1.1f)});
	}
	Ridge.push_back({W + 20.0f, Band.Y + Band.H});
	Ridge.push_back({0.0f, Band.Y + Band.H});
	C.FillPolygon(Ridge, Hex(rl::EcSlate, 0.8f));
	C.PopClip();
	C.FillRect({0.0f, Band.Y + Band.H - 4.0f, W, 4.0f}, Hex(rl::EcEmberHot));
	rl::NetEmbercrest(C, 84.0f, 84.0f, 104.0f, Hex(rl::EcMidnight));
	rl::NetSpaced(C, "THE EMBERCREST", 150.0f, 82.0f, 31.0f, 900, Hex(rl::EcAsh), 2.4f);
	rl::NetSpaced(C, "CASINO \xC2\xB7 CARD ROOM", 152.0f, 106.0f, 13.0f, 800, Hex(rl::EcEmberHot), 2.6f);
	C.Text("Poker every day \xC2\xB7 6-max freezeouts \xC2\xB7 deep stacks", 152.0f, 128.0f, Ts(12.5f, 600, Hex(rl::EcIce)));

	// The month, the days gone by crossed off in pen.
	int Year = 0;
	int Month = 0;
	int Date = 0;
	world::CivilDate(Today, Year, Month, Date);
	static const char* Months[12] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
	rl::NetSpaced(C, std::string(Months[(Month + 11) % 12]) + " " + std::to_string(Year), W * 0.5f, 188.0f, 22.0f, 900, Print, 3.0f, Align::Center);
	static const char* Initials[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
	for (int K = 0; K < 7; ++K)
	{
		C.Text(Initials[K], GridX + (F(K) + 0.5f) * Cell, 214.0f, Ts(11.0f, 800, K >= 5 ? Hex(rl::EcEmber) : Hex(0x6b6a66), Align::Center));
	}
	const int First = Today - (Date - 1);
	int Days = 28;
	for (int D = 28; D <= 31; ++D)
	{
		int Y2 = 0;
		int M2 = 0;
		int D2 = 0;
		world::CivilDate(First + D - 1, Y2, M2, D2);
		Days = M2 == Month ? D : Days;
	}
	const int Lead = ((First % 7) + 7) % 7; // day 0 is a Monday
	const int Rows = (Lead + Days + 6) / 7;
	const float Top = 224.0f;
	const float RowH = std::min(40.0f, 200.0f / F(Rows));
	const bool RentLive = L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice;
	const int RentDay = net::DayOf(L.RentDeadline - 1.0);
	const std::map<int, std::string> Written = MonthMarks(S, Today);
	for (int R = 0; R <= Rows; ++R)
	{
		C.FillRect({GridX, Top + F(R) * RowH, GridW, 1.0f}, Rgba(27, 34, 56, 0.18f));
	}
	for (int K = 0; K <= 7; ++K)
	{
		C.FillRect({GridX + F(K) * Cell, Top, 1.0f, F(Rows) * RowH}, Rgba(27, 34, 56, 0.12f));
	}
	for (int D = 0; D < Days; ++D)
	{
		const int Day = First + D;
		const int Slot = Lead + D;
		const float Cx = GridX + F(Slot % 7) * Cell;
		const float Cy = Top + F(Slot / 7) * RowH;
		if (Day == Today)
		{
			// Today, gone over with a highlighter.
			C.FillRect({Cx + 3.0f, Cy + 3.0f, Cell - 6.0f, RowH - 6.0f}, Rgba(255, 222, 70, 0.42f));
		}
		C.Text(std::to_string(D + 1), Cx + 7.0f, Cy + 17.0f, Ts(15.0f, Day == Today ? 900 : 700, Day == Today ? Marker : Print));
		// The room's events that day, a dot each in its colour; the player's night marked IN (or how it went).
		const std::vector<live::Occurrence> Os = live::Occurrences(Day);
		for (size_t K = 0; K < Os.size(); ++K)
		{
			C.FillCircle(Cx + Cell - 9.0f - F(static_cast<int>(K)) * 9.0f, Cy + RowH - 8.0f, 3.0f, rl::LiveColor(*Os[K].T));
			if (const life::LiveEntry* E = live::EntryFor(L, Os[K].Id))
			{
				const std::string Mark = E->State == life::LiveEntry::Registered ? std::string("IN") : E->State == life::LiveEntry::Refunded ? std::string("OFF") : Ordinal(E->Place);
				C.Text(Mark, Cx + 7.0f, Cy + RowH - 5.0f, Ts(11.0f, 900, E->State == life::LiveEntry::Registered || E->PrizeCents > 0 ? Marker : rl::NetA(Marker, 0.6f)));
			}
		}
		if (RentLive && Day == RentDay)
		{
			C.Text("RENT", Cx + Cell - 6.0f, Cy + 17.0f, Ts(11.0f, 900, Marker, Align::Right));
			C.StrokeRoundRect({Cx + 2.0f, Cy + 2.0f, Cell - 4.0f, RowH - 4.0f}, 3.0f, Marker, 2.5f);
		}
		else if (const auto Mk = Written.find(Day); Mk != Written.end())
		{
			// What else is due that day, in ballpoint: a payment, a chop, a sponsor's review.
			C.Text(Mk->second, Cx + Cell - 6.0f, Cy + 16.0f, Ts(10.0f, 900, Pen, Align::Right));
		}
		if (Day < Today)
		{
			C.StrokePolyline({{Cx + 6.0f, Cy + RowH - 6.0f}, {Cx + Cell - 6.0f, Cy + 6.0f}}, false, rl::NetA(Pen, 0.55f), 2.0f, true);
		}
	}

	// Today and tomorrow, big enough to read from the chair.
	float Y = Top + F(Rows) * RowH + 34.0f;
	Y = DayBlock(C, S, Today, "TODAY \xC2\xB7 " + rl::NetUpper(net::WeekdayName(Today)) + " " + std::to_string(Date), Y, World) + 10.0f;
	int Y3 = 0;
	int M3 = 0;
	int Tomorrow = 0;
	world::CivilDate(Today + 1, Y3, M3, Tomorrow);
	DayBlock(C, S, Today + 1, "TOMORROW \xC2\xB7 " + rl::NetUpper(net::WeekdayName(Today + 1)) + " " + std::to_string(Tomorrow), Y, World);

	// The rent, in marker along the bottom.
	std::string Rent;
	if (L.RentStage == life::Rent::Evicted)
	{
		Rent = "LOCKED OUT \xC2\xB7 KEY BACK " + rl::NetMoney(L.RentDueCents);
	}
	else if (L.RentStage == life::Rent::Paid)
	{
		Rent = "RENT PAID \xC2\xB7 NEXT DUE " + rl::NetUpper(net::DateLabel(RentDay));
	}
	else
	{
		const int Left = RentDay - Today;
		Rent = (L.RentStage == life::Rent::FinalNotice ? "LAST CHANCE " : "RENT ") + rl::NetMoney(L.RentDueCents) + " \xC2\xB7 " +
			(Left <= 0 ? std::string("TONIGHT") : Left == 1 ? std::string("TOMORROW NIGHT") : rl::NetUpper(net::WeekdayName(RentDay, true)) + " " + rl::NetUpper(net::DateLabel(RentDay)));
	}
	C.Text(Rent, GridX, H - 26.0f, Ts(19.0f, 900, L.RentStage == life::Rent::Paid ? Pen : Marker, Align::Left, Baseline::Alphabetic, false, GridW));
	C.FillRect({GridX, H - 19.0f, std::min(GridW, C.Measure(Rent, 19.0f, 900)), 2.5f}, L.RentStage == life::Rent::Paid ? rl::NetA(Pen, 0.6f) : rl::NetA(Marker, 0.7f));
}
} // namespace wallcalendar
} // namespace ui
} // namespace ss
