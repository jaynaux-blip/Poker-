// RiverLine's network lobby: the schedule, the series, the leaderboards, the news and the player's career.
// Everything here reads the simulated network (ShortStack/Game/Network.h) at the session's world clock.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/ShopArt.h"
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"
#include "ShortStack/UI/BadgeArt.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlnet_detail
{
/** Who pays for an online event if the player registers now (Session::FundingOptions' first that can): a package's roll, else their own money. */
life::Funding OnlineFunding(const Session& S, const std::string& EventId)
{
	for (const std::pair<life::Funding, std::string>& Option : S.FundingOptions(EventId))
	{
		if (Option.second.empty())
		{
			return Option.first;
		}
	}
	return life::Funding();
}
} // namespace rlnet_detail

namespace backing_detail
{
// RiverLineBacking.cpp (core-stake): opens the sell panel on an event (an occurrence or an online event), the share and
// markup at their start.
void Preselect(const Ui& UI, const std::string& EventId);
} // namespace backing_detail

// ------------------------------------------------------------------ frame and navigation

void RiverLine::NetFrame(double Now)
{
	if (LastFrame < 0.0)
	{
		SlideAt = Now;
	}
	Dt = LastFrame < 0.0 ? 0.0 : std::min(0.1, std::max(0.0, Now - LastFrame));
	LastFrame = Now;
	World = S.WorldMinutes();
	net::Shared().SetHero(S.HeroName, S.History);
	You = net::StatsFrom(S.HeroName, S.History, World);
	You.LivePoints = S.Living().HeroSeasonLivePoints();
	if (NewsSeenAt == 0.0)
	{
		NewsSeenAt = World - 6.0 * 60.0;
	}
}

void RiverLine::OpenPage(Page P, double Now)
{
	if (P != PageShown)
	{
		PageAt = Now;
		if (P == Page::Embercrest)
		{
			// The page opens on the week with every card on its facts: the small choices last a visit (they're kept by
			// address, NetChoicesOf, so a new laptop never inherits an old one's), and the flips don't pile up.
			NetChoices& Ch = NetChoicesOf(this);
			Ch.WeekTab = 0;
			Ch.WeekTabAt = -100.0;
			Ch.Flip.clear();
		}
	}
	PageShown = P;
	S.ConfirmRegister = false;
	LiveConfirm.clear();
	if (P == Page::News)
	{
		NewsSeenAt = World;
	}
	if (P == Page::Leaderboards)
	{
		BoardAt = Now;
		BoardScroll = BoardScrollGoal = 0.0f;
	}
	if (P == Page::Series)
	{
		SeriesAt = Now;
	}
}

void RiverLine::SetFilter(Filter F, double Now)
{
	if (F != FilterShown)
	{
		FilterAt = Now;
		ListScroll = ListScrollGoal = 0.0f;
	}
	FilterShown = F;
}

void RiverLine::SelectEvent(const std::string& InstanceId)
{
	if (InstanceId != EventId)
	{
		EventAt = LastFrame;
		DetailTab = 0;
		S.ConfirmRegister = false;
	}
	EventId = InstanceId;
	ScrollToSelected = true;
}

void RiverLine::ShowBoard(net::Board B, double Now)
{
	if (B != BoardShown || HonorsShown)
	{
		BoardAt = Now;
		BoardScroll = BoardScrollGoal = 0.0f;
	}
	BoardShown = B;
	HonorsShown = false;
}

void RiverLine::ShowSeries(const std::string& Id, double Now)
{
	if (Id != SeriesId)
	{
		SeriesAt = Now;
		SeriesDay = -9999;
	}
	SeriesId = Id;
}

void RiverLine::ShowLiveDay(int Day, const std::string& OccurrenceId, double Now)
{
	if (Day != LiveDay)
	{
		LiveDayAt = Now;
		LiveConfirm.clear();
	}
	LiveDay = Day;
	LivePick = OccurrenceId;
}

const std::vector<net::NewsItem>& RiverLine::NewsFeed()
{
	const long long Key = static_cast<long long>(std::floor(World)) * 1000 + static_cast<long long>(S.History.size());
	if (Key != NewsKey)
	{
		NewsKey = Key;
		News = net::Shared().News(World, You, 24);
	}
	return News;
}

void RiverLine::NavTabs(double Now)
{
	const net::Network& Net = net::Shared();
	const bool InLobby = S.CurrentScreen == Screen::Lobby;
	// At the tables the pages are a click away; the tables keep playing.
	const bool Navigable = InLobby || (S.CurrentScreen == Screen::Table && S.T);
	// The pages, and Backing (next_segment_spec §2.12) once there's anything to show on it: an offer, a deal, a listing,
	// a swap or something owed. The page stays reachable while it's up, so a last deal settling doesn't pull it away.
	const bool Backing = stake::Anything(S.Life.Stakes) || (InLobby && PageShown == Page::Backing);
	const int Count = Backing ? 7 : 6;
	// With two or more tables open their tabs share the bar: the pages tighten up so four tables and Backing still fit.
	const bool Tight = S.TableCount() >= 2;
	const float Size = Tight ? 15.0f : 17.0f;
	const float Gap = Tight ? 20.0f : 30.0f;
	const char* Names[7] = {"Lobby", "Series", Tight ? "Boards" : "Leaderboards", "News", "Career", "Embercrest", "Backing"};
	float X = 250.0f;
	float TargetX = NavX;
	float TargetW = NavW;
	int Unread = 0;
	if (InLobby)
	{
		for (const net::NewsItem& It : NewsFeed())
		{
			Unread += It.At > NewsSeenAt ? 1 : 0;
		}
	}
	for (int I = 0; I < Count; ++I)
	{
		const float Tw = UI.Measure(Names[I], Size, 700);
		const Ui::ClickState St = UI.Clickable(std::string("nav") + std::to_string(I), {X - Gap * 0.4f, 10.0f, Tw + Gap * 0.8f, NetTop - 12.0f}, Navigable);
		if (St.Clicked)
		{
			S.ShowLobby();
			OpenPage(static_cast<Page>(I), Now);
		}
		const bool Active = InLobby && PageShown == static_cast<Page>(I);
		const Color Col = !Navigable ? pal::Dim : Active ? pal::Ink : St.Hover ? Hex(0xc3cedf) : InLobby ? pal::Muted : pal::Dim;
		UI.Text(Names[I], X, 40.0f, Ts(Size, Active ? 700 : 500, Col));
		if (Active)
		{
			TargetX = X;
			TargetW = Tw;
		}
		float Extra = 0.0f;
		if (I == 1)
		{
			if (const net::SeriesInfo* Sr = Net.CurrentSeries(World))
			{
				if (net::DayOf(World) >= Sr->FirstDay && InLobby)
				{
					const float A0 = C->GetAlpha();
					C->SetAlpha(A0 * (0.75f + 0.25f * Nf(std::sin(Now * 3.0))));
					Extra = NetPill(*C, "LIVE", X + Tw + 7.0f, 18.0f, Hex(Sr->Color), true, 9.0f) + 7.0f;
					C->SetAlpha(A0);
				}
			}
		}
		if (I == 3 && Unread > 0)
		{
			const std::string N = std::to_string(std::min(Unread, 9));
			C->FillCircle(X + Tw + 13.0f, 26.0f, 9.0f, pal::Red);
			UI.Text(N, X + Tw + 13.0f, 26.5f, Ts(11.0f, 800, Hex(0xffffff), Align::Center, Baseline::Middle));
			Extra = 22.0f;
		}
		if (I == 5 && live::ActiveEntry(S.Life))
		{
			// Registered at the Embercrest: its ember by the name.
			C->FillCircle(X + Tw + 10.0f, 26.0f, 4.5f, Hex(EcEmberHot));
			Extra = 14.0f;
		}
		if (I == 6)
		{
			// Backing: an offer waiting on an answer (gold), or money of theirs past due (red).
			bool Waiting = false;
			for (const stake::Offer& O : S.Life.Stakes.Offers)
			{
				Waiting = Waiting || (O.State == stake::OfferStanding && O.Expires > World);
			}
			bool Late = false;
			for (const stake::Owed& O : S.Life.Stakes.Owes)
			{
				Late = Late || (!O.ToYou && O.State == stake::OwedLate);
			}
			if (Late || Waiting)
			{
				C->FillCircle(X + Tw + 10.0f, 26.0f, 4.5f, Late ? pal::Red : pal::Gold);
				Extra = 14.0f;
			}
		}
		X += Tw + Gap + Extra;
	}
	if (InLobby && TargetW > 0.0f)
	{
		if (NavX < 0.0f)
		{
			NavX = TargetX;
			NavW = TargetW;
		}
		const float K = NetFollow(Dt, 16.0);
		NavX += (TargetX - NavX) * K;
		NavW += (TargetW - NavW) * K;
		C->GlowRoundRect({NavX - 4.0f, NetTop - 7.0f, NavW + 8.0f, 8.0f}, 4.0f, NetA(pal::Accent, 0.35f), 8.0f);
		UI.RRect({NavX, NetTop - 4.0f, NavW, 3.0f}, 1.5f, pal::Accent);
	}
	// The tables being played: tabs that flash when one needs the player.
	TableStrip(X, Now);
}

void RiverLine::Panel(const Rect& R, float Radius)
{
	C->GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, Radius, Rgba(0, 0, 0, 0.35f), 18.0f);
	UI.RRect(R, Radius, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x122036), Hex(0x0d1729)), pal::Line);
	C->FillRect({R.X + Radius, R.Y + 1.0f, R.W - Radius * 2.0f, 1.0f}, Rgba(255, 255, 255, 0.05f));
}

float RiverLine::Section(const std::string& Title, float X, float Y, const Color& Col)
{
	C->FillRoundRect({X, Y - 11.0f, 3.0f, 13.0f}, 1.5f, Col);
	return NetSpaced(*C, Title, X + 11.0f, Y, 12.0f, 800, pal::Muted, 1.6f) + 11.0f;
}

std::string RiverLine::PlacingName(const net::Placing& P) const
{
	return P.Player == -1 ? S.HeroName : P.Player == -2 ? P.Name : net::Shared().Players()[static_cast<size_t>(P.Player)].Name;
}

void RiverLine::PlayerName(const net::Placing& P, float X, float Y, float Size, float MaxW, bool Badges, uint8_t Frame)
{
	if (P.Player != -2)
	{
		PlayerName(P.Player, X, Y, Size, MaxW, Badges, Frame);
		return;
	}
	// Someone nobody follows: just a name and a flag.
	NetFlag(*C, P.Country, X, Y - Size * 0.72f, Size * 1.2f, Size * 0.8f);
	UI.Text(P.Name, X + Size * 1.2f + 8.0f, Y, Ts(Size, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, MaxW - Size * 1.2f - 8.0f));
}

void RiverLine::PlayerName(int Index, float X, float Y, float Size, float MaxW, bool Badges, uint8_t Frame)
{
	const net::Network& Net = net::Shared();
	// After the name (rule C1): one badge glyph, the first in the network's order that the avatar's frame doesn't
	// already say (C2), then at most one relationship pill. Hovering the glyph shows what it is (BadgeTip).
	const float Glyph = std::round(Size + 1.0f);
	auto BadgeAfter = [&](float Gx, int Npc) -> float {
		const badge::Badge* B = badge::Top(NetBadgesOf(S, Npc), badge::Network, Frame);
		if (!B)
		{
			return 0.0f;
		}
		const float Cy = Y - Size * 0.36f;
		badgeart::Draw(*C, *B, Gx + Glyph / 2.0f, Cy, Glyph, LastFrame);
		if (UI.Hover({Gx - 2.0f, Cy - Glyph / 2.0f - 2.0f, Glyph + 4.0f, Glyph + 4.0f}))
		{
			NetTip& Tip = NetTipOf(this);
			Tip.Up = true;
			Tip.B = *B;
			Tip.Npc = Npc;
			Tip.X = Gx + Glyph / 2.0f;
			Tip.Y = Cy + Glyph / 2.0f;
		}
		return Glyph + 6.0f;
	};
	if (Index < 0)
	{
		const float W = UI.Text(S.HeroName, X, Y, Ts(Size, 800, pal::Accent, Align::Left, Baseline::Alphabetic, false, MaxW));
		if (Badges)
		{
			const float Gw = BadgeAfter(X + W + 6.0f, -1);
			NetPill(*C, "YOU", X + W + 8.0f + Gw, Y - Size * 0.78f, pal::Accent, true, 9.0f);
		}
		return;
	}
	const net::Player& P = Net.Players()[static_cast<size_t>(Index)];
	NetFlag(*C, P.Country, X, Y - Size * 0.72f, Size * 1.2f, Size * 0.8f);
	const float Nx = X + Size * 1.2f + 8.0f;
	// The name opens their player card.
	const float Fit = std::min(MaxW - Size * 1.2f - 8.0f, UI.Measure(P.Name, Size, 700));
	const Ui::ClickState Click = UI.Clickable("player:" + std::to_string(Index) + "@" + std::to_string(static_cast<int>(X)) + "," + std::to_string(static_cast<int>(Y)), {Nx - 2.0f, Y - Size, Fit + 4.0f, Size * 1.3f},
		S.CurrentScreen == Screen::Lobby && !CardOpen());
	if (Click.Clicked)
	{
		ShowPlayer(Index, LastFrame);
	}
	const float W = UI.Text(P.Name, Nx, Y, Ts(Size, 700, P.Rival ? Hex(0xd5b8ff) : Click.Hover ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW - Size * 1.2f - 8.0f));
	if (Click.Hover)
	{
		C->FillRect({Nx, Y + 3.0f, std::min(W, Fit), 1.5f}, pal::Accent);
	}
	if (!Badges)
	{
		return;
	}
	// The glyph (a pro's is the stitched patch, which took the place of the TEAM RL pill), then RIVAL.
	const float Gw = BadgeAfter(Nx + W + 6.0f, Index);
	const float Px = Nx + W + 8.0f + Gw;
	if (P.Rival)
	{
		NetPill(*C, "RIVAL", Px, Y - Size * 0.78f, Hex(0xb36bff), true, 9.0f);
	}
	else if (P.Pro && Gw <= 0.0f && Frame != badge::FrameGold)
	{
		// Team RiverLine with no badge to show for it here (and no gold frame beside the name saying so).
		NetPill(*C, "TEAM RL", Px, Y - Size * 0.78f, pal::Gold, false, 9.0f);
	}
	else if (Gw <= 0.0f)
	{
		// New on RiverLine this month: the kind of name that catches the eye (only when there's no glyph, C1).
		const world::Npc* N = S.Living().Get(Index);
		if (N && N->Came != world::Arrival::None && net::DayOf(World) - N->Arrived <= 30)
		{
			NetPill(*C, "NEW", Px, Y - Size * 0.78f, pal::Accent, true, 9.0f);
		}
	}
}

// ------------------------------------------------------------------ lobby

void RiverLine::NetLobby(double Now)
{
	const net::Network& Net = net::Shared();
	const std::vector<ListRow> Rows = ScheduleRows();
	// Something is always selected: the first event the player can join, or the top of the list.
	net::EventInstance Current;
	if (EventId.empty() || !Net.FindInstance(EventId, Current))
	{
		for (const ListRow& Row : Rows)
		{
			if (Row.Joinable && S.BankrollCents >= Net.TemplateOf(Row.E).BuyInCents && EventId.empty())
			{
				EventId = Row.E.Id;
			}
		}
		if (EventId.empty() && !Rows.empty())
		{
			EventId = Rows.front().E.Id;
		}
	}
	Featured({24.0f, 84.0f, 1052.0f, 214.0f}, Now);
	FilterChips(24.0f, 314.0f, 1052.0f, Now, static_cast<int>(Rows.size()));
	Schedule({24.0f, 362.0f, 1052.0f, 588.0f}, Rows, Now);
	EventPanel({1092.0f, 84.0f, 484.0f, 866.0f}, Now);
}

const std::vector<RiverLine::FeatureSlide>& RiverLine::FeatureSlides()
{
	const long long Key = static_cast<long long>(std::floor(World));
	if (Key == SlidesKey)
	{
		return Slides;
	}
	SlidesKey = Key;
	Slides.clear();
	const net::Network& Net = net::Shared();
	// Tonight's featured event you can play: the series event first.
	const net::EventInstance* Tonight = nullptr;
	const std::vector<net::EventInstance> Near = Net.Window(World - 8.0 * 60.0, World + 3.0 * 60.0);
	for (const net::EventInstance& E : Near)
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::Status St = Net.Live(E, World).St;
		if (T.Featured && T.JoinIndex >= 0 && (St == net::Status::LateReg || St == net::Status::Registering))
		{
			if (!Tonight || (!T.Series.empty() && Net.TemplateOf(*Tonight).Series.empty()))
			{
				Tonight = &E;
			}
		}
	}
	if (Tonight)
	{
		const net::EventTemplate& T = Net.TemplateOf(*Tonight);
		const net::LiveState L = Net.Live(*Tonight, World);
		FeatureSlide F;
		F.Art = 0;
		F.HasEvent = true;
		F.E = *Tonight;
		if (const net::SeriesInfo* Sr = Net.FindSeries(T.Series))
		{
			F.Col = Sr->Color;
			F.Col2 = Sr->Color2;
			F.Kicker = NetUpper(Sr->Name) + " \xC2\xB7 EVENT #" + std::to_string(T.EventNo);
			F.Title = NetShortName(T.Name);
		}
		else
		{
			F.Kicker = "TONIGHT ON RIVERLINE";
			F.Title = T.Name;
		}
		F.Sub = net::BuyIn(T.BuyInCents) + " buy-in \xC2\xB7 NL Hold'em " + T.Speed + " \xC2\xB7 " + Grouped(Tonight->Entries) + " players";
		F.Big = NetMoney(T.GtdCents > 0 ? T.GtdCents : Tonight->Pool);
		F.BigLabel = T.GtdCents > 0 ? "GUARANTEED" : "PRIZE POOL";
		F.ClockLabel = L.St == net::Status::LateReg ? "LATE REGISTRATION ENDS IN" : "STARTS IN";
		F.ClockAt = L.St == net::Status::LateReg ? Tonight->Start + Tonight->LateReg : Tonight->Start;
		F.Cta = "Register \xC2\xB7 " + net::BuyIn(T.BuyInCents);
		F.Action = 0;
		Slides.push_back(F);
	}
	net::EventInstance E;
	if (Net.Next("mm-main", World - 12.0 * 60.0, E) && Net.Live(E, World).St != net::Status::Finished)
	{
		const net::SeriesInfo* Sr = Net.FindSeries("mm");
		FeatureSlide F;
		F.Art = 1;
		F.HasEvent = true;
		F.E = E;
		F.SeriesId = "mm";
		F.Col = Sr ? Sr->Color : F.Col;
		F.Col2 = Sr ? Sr->Color2 : F.Col2;
		F.Kicker = "MICRO MADNESS \xC2\xB7 MAIN EVENT";
		F.Title = "$11 to play for a million";
		F.Sub = std::string(net::WeekdayName(net::DayOf(E.Start), true)) + ", " + net::DateLabel(net::DayOf(E.Start)) + " \xC2\xB7 " + net::TimeLabel(E.Start) + " \xC2\xB7 50,000 chips \xC2\xB7 12-minute levels";
		F.Big = "$1,000,000";
		F.BigLabel = "GUARANTEED";
		F.ClockLabel = "STARTS IN";
		F.ClockAt = E.Start;
		F.Cta = "View the series";
		F.Action = 1;
		Slides.push_back(F);
	}
	if (const net::SeriesInfo* Rc = Net.FindSeries("rcop"))
	{
		if (net::DayOf(World) <= Rc->LastDay)
		{
			FeatureSlide F;
			F.Art = 2;
			F.SeriesId = Rc->Id;
			F.Col = Rc->Color;
			F.Col2 = Rc->Color2;
			F.Kicker = NetUpper(net::DateLabel(Rc->FirstDay)) + " \xE2\x80\x93 " + NetUpper(net::DateLabel(Rc->LastDay));
			F.Title = Rc->Name;
			F.Sub = std::to_string(Rc->Events) + " events \xC2\xB7 $5,250 Main Event with $25,000,000 guaranteed";
			F.Big = "$100,000,000";
			F.BigLabel = "GUARANTEED";
			F.ClockLabel = net::DayOf(World) < Rc->FirstDay ? "SERIES STARTS IN" : "SERIES ENDS IN";
			F.ClockAt = net::DayOf(World) < Rc->FirstDay ? static_cast<double>(Rc->FirstDay) * net::MinutesPerDay + 11.0 * 60.0 : static_cast<double>(Rc->LastDay + 1) * net::MinutesPerDay;
			F.Cta = "Road to the Main Event";
			F.Action = 1;
			Slides.push_back(F);
		}
	}
	// The series on now or coming up (every year's calendar after RCOP 2026).
	if (const net::SeriesInfo* Cur = Net.CurrentSeries(World))
	{
		const int Today = net::DayOf(World);
		if (Cur->Id != "mm" && Cur->Id != "rcop" && Cur->Id != "slam" && Today >= Cur->FirstDay - 21 && Today <= Cur->LastDay)
		{
			FeatureSlide F;
			F.Art = Cur->Bracelets > 0 || Cur->Rings > 0 ? 1 : 2;
			F.SeriesId = Cur->Id;
			F.Col = Cur->Color;
			F.Col2 = Cur->Color2;
			F.Kicker = NetUpper(net::DateLabel(Cur->FirstDay)) + " \xE2\x80\x93 " + NetUpper(net::DateLabel(Cur->LastDay)) +
				(Cur->Bracelets > 0 ? " \xC2\xB7 " + std::to_string(Cur->Bracelets) + " BRACELETS" : Cur->Rings > 0 ? " \xC2\xB7 " + std::to_string(Cur->Rings) + " RINGS" : "");
			F.Title = Cur->Name;
			const net::EventTemplate* Main = Net.FindTemplate(Cur->MainEvent);
			F.Sub = std::to_string(Cur->Events) + " events" + (Main ? " \xC2\xB7 " + NetShortName(Main->Name) + ", " + NetMoney(Main->GtdCents) + " guaranteed" : std::string());
			F.Big = NetMoney(Cur->GtdCents);
			F.BigLabel = "GUARANTEED";
			F.ClockLabel = Today < Cur->FirstDay ? "SERIES STARTS IN" : "SERIES ENDS IN";
			F.ClockAt = Today < Cur->FirstDay ? static_cast<double>(Cur->FirstDay) * net::MinutesPerDay + 12.0 * 60.0 : static_cast<double>(Cur->LastDay + 1) * net::MinutesPerDay;
			F.Cta = Cur->Bracelets > 0 ? "The bracelet events" : Cur->Rings > 0 ? "The ring events" : "View the series";
			F.Action = 1;
			Slides.push_back(F);
		}
	}
	if (Net.Next("millions", World, E))
	{
		FeatureSlide F;
		F.Art = 4;
		F.HasEvent = true;
		F.E = E;
		F.Col = 0xf2c14e;
		F.Col2 = 0xf28a3a;
		F.Kicker = "SUNDAY MAJORS";
		F.Title = "RiverLine Millions";
		F.Sub = "$1,050 \xC2\xB7 " + std::string(net::WeekdayName(net::DayOf(E.Start), true)) + " " + net::TimeLabel(E.Start) + " \xC2\xB7 the best in the world at every table";
		F.Big = "$3,000,000";
		F.BigLabel = "GUARANTEED";
		F.ClockLabel = "STARTS IN";
		F.ClockAt = E.Start;
		F.Cta = "Event details";
		F.Action = 2;
		Slides.push_back(F);
	}
	const double ShiftEnd = life::NightShiftStart(World) + 12.0 * 60.0;
	if (World < ShiftEnd)
	{
		FeatureSlide F;
		F.Art = 3;
		F.Col = 0x27d3c3;
		F.Col2 = 0x8b5cf6;
		F.Kicker = "TONIGHT'S LEADERBOARD";
		F.Title = "The Night Shift";
		F.Sub = "Every micro-stakes final table from 6 PM to 6 AM scores. The top 20 share the pot.";
		F.Big = "$1,000";
		F.BigLabel = "IN PRIZES";
		F.ClockLabel = "ENDS IN";
		F.ClockAt = ShiftEnd;
		F.Cta = "View standings";
		F.Action = 3;
		Slides.push_back(F);
	}
	return Slides;
}

void RiverLine::DrawSlide(const FeatureSlide& F, const Rect& R, double Now, float Alpha, float Offset, bool Interactive)
{
	const net::Network& Net = net::Shared();
	const Color Col = Hex(F.Col);
	const Color Col2 = Hex(F.Col2);
	const float A0 = C->GetAlpha();
	C->Save();
	C->SetAlpha(A0 * Alpha);
	C->Translate(Offset, 0.0f);
	C->FillRoundRect(R, 16.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(Hex(0x07101c), Col, 0.2f), Mix(Hex(0x0a1322), Col2, 0.16f)));
	// Light: a glow behind the art and a slow sweep across the card.
	const float Gx = R.X + R.W - 250.0f;
	const float Gy = R.Y + R.H * 0.5f;
	C->FillEllipse(Gx, Gy, 340.0f, 230.0f, Paint::Radial({Gx, Gy}, 0.0f, {Gx, Gy}, 340.0f, NetA(Col, 0.42f), 0.45f, NetA(Col2, 0.12f), NetA(Col2, 0.0f)));
	const float Sweep = R.X - 300.0f + Nf(std::fmod(Now * 150.0, static_cast<double>(R.W) + 900.0));
	C->FillPolygon({{Sweep, R.Y}, {Sweep + 90.0f, R.Y}, {Sweep + 20.0f, R.Y + R.H}, {Sweep - 70.0f, R.Y + R.H}}, Rgba(255, 255, 255, 0.035f));
	for (int K = 0; K < 3; ++K)
	{
		std::vector<Vec2> Wave;
		for (float X = R.X; X <= R.X + R.W; X += 12.0f)
		{
			Wave.push_back({X, R.Y + R.H - 40.0f + Nf(K) * 12.0f + Nf(std::sin(X / 120.0f + Now * 0.8 + K)) * 10.0f});
		}
		C->StrokePolyline(Wave, false, NetA(K % 2 ? Col2 : Col, 0.12f), 1.5f);
	}
	// Art: the event's emblem or the series' crest when there is one; otherwise the slide's own picture.
	const net::SeriesInfo* ArtSeries = !F.HasEvent && !F.SeriesId.empty() ? Net.FindSeries(F.SeriesId) : nullptr;
	if (F.HasEvent || ArtSeries)
	{
		const float Ex = Gx - 30.0f;
		const float Ey = Gy - 16.0f + Nf(std::sin(Now * 1.1)) * 3.0f;
		for (int K = 0; K < 3; ++K)
		{
			const float Ph = Nf(std::fmod(Now * 0.22 + K / 3.0, 1.0));
			C->StrokeEllipse(Ex, Ey, 70.0f + Ph * 150.0f, 70.0f + Ph * 150.0f, NetA(K % 2 ? Col2 : Col, 0.32f * (1.0f - Ph)), 1.5f);
		}
		if (F.Art == 0)
		{
			const Chips Amounts[2] = {600000, 1200000};
			DrawChipStack(*C, Amounts[0], Ex - 120.0f, Ey + 52.0f, 1.3f, C->GetAlpha());
			DrawChipStack(*C, Amounts[1], Ex + 96.0f, Ey + 58.0f, 1.3f, C->GetAlpha());
		}
		if (ArtSeries)
		{
			eventart::SeriesCrest(*C, *ArtSeries, Ex, Ey, 184.0f, Now);
		}
		else
		{
			eventart::Emblem(*C, Net.TemplateOf(F.E), Ex, Ey, F.Art == 4 ? 150.0f : 176.0f, Now);
		}
	}
	switch (F.HasEvent || ArtSeries ? -1 : F.Art)
	{
	case -1:
		break;
	case 0:
	{
		const Chips Amounts[5] = {250000, 1200000, 600000, 80000, 2500000};
		const float Xs[5] = {-120.0f, -40.0f, 40.0f, 120.0f, 0.0f};
		const float Ys[5] = {30.0f, 10.0f, 34.0f, 16.0f, -24.0f};
		for (int I = 0; I < 5; ++I)
		{
			DrawChipStack(*C, Amounts[I], Gx - 90.0f + Xs[I], Gy - 22.0f + Ys[I], 1.5f, C->GetAlpha());
		}
		break;
	}
	case 1:
		C->GlowRoundRect({Gx - 60.0f, Gy - 70.0f, 120.0f, 140.0f}, 60.0f, NetA(Col, 0.25f), 40.0f);
		NetTrophy(*C, Gx, Gy - 78.0f, 150.0f, Mix(Col, Hex(0xffffff), 0.35f), Col2);
		break;
	case 2:
		for (int K = 0; K < 4; ++K)
		{
			const float Ph = Nf(std::fmod(Now * 0.25 + K * 0.25, 1.0));
			C->StrokeEllipse(Gx, Gy, 40.0f + Ph * 200.0f, (40.0f + Ph * 200.0f) * 0.62f, NetA(K % 2 ? Col2 : Col, 0.5f * (1.0f - Ph)), 2.0f);
		}
		NetStar(*C, Gx, Gy, 34.0f, Col);
		break;
	case 3:
	{
		const float Hs[3] = {70.0f, 100.0f, 52.0f};
		const Color Medal[3] = {Hex(0xc9d3e0), Hex(0xf2c14e), Hex(0xd08c4f)};
		for (int I = 0; I < 3; ++I)
		{
			const float Bx = Gx - 105.0f + Nf(I) * 70.0f;
			const float H = Hs[I] * (0.85f + 0.15f * Nf(std::sin(Now * 1.2 + I)));
			C->FillRoundRect({Bx, Gy + 60.0f - H, 60.0f, H}, 6.0f, Paint::Linear({0.0f, Gy + 60.0f - H}, {0.0f, Gy + 60.0f}, Medal[I], NetA(Medal[I], 0.2f)));
			UI.Text(std::to_string(I == 1 ? 1 : I == 0 ? 2 : 3), Bx + 30.0f, Gy + 60.0f - H + 30.0f, Ts(24.0f, 900, Hex(0x0a1322), Align::Center));
		}
		NetCrown(*C, Gx, Gy - 52.0f, 34.0f, pal::Gold);
		break;
	}
	default:
		C->GlowRoundRect({Gx - 50.0f, Gy - 50.0f, 100.0f, 100.0f}, 50.0f, NetA(Col, 0.3f), 40.0f);
		NetCrown(*C, Gx, Gy + 30.0f, 110.0f, Col);
		break;
	}
	// Words.
	const float Lx = R.X + 36.0f;
	NetSpaced(*C, F.Kicker, Lx, R.Y + 42.0f, 12.0f, 800, Col, 2.0f);
	UI.Text(F.Title, Lx, R.Y + 88.0f, Ts(40.0f, 900, pal::Ink, Align::Left, Baseline::Alphabetic, false, 560.0f));
	UI.Text(F.Sub, Lx, R.Y + 116.0f, Ts(16.0f, 500, Hex(0xb9c4d6), Align::Left, Baseline::Alphabetic, false, 600.0f));
	NetSpaced(*C, F.ClockLabel, Lx, R.Y + 156.0f, 11.0f, 700, pal::Muted, 1.5f);
	UI.Text(NetHms(F.ClockAt - World), Lx, R.Y + 190.0f, Ts(30.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	// Big number.
	const float Bx = R.X + R.W - 36.0f;
	const float Bw = UI.Measure(F.Big, 46.0f, 900);
	C->GlowRoundRect({Bx - Bw, R.Y + 146.0f, Bw, 40.0f}, 20.0f, Rgba(0, 0, 0, 0.45f), 26.0f);
	UI.Text(F.Big, Bx, R.Y + 186.0f, Ts(46.0f, 900, Hex(0xffffff), Align::Right));
	NetSpaced(*C, F.BigLabel, Bx, R.Y + 138.0f, 11.0f, 800, Col, 2.0f, Align::Right);
	// Call to action.
	if (!F.Cta.empty())
	{
		ButtonOpts O;
		O.Kind = F.Action == 0 ? ButtonKind::Primary : ButtonKind::Secondary;
		O.Size = 17.0f;
		O.Enabled = Interactive;
		const float Cw = std::max(190.0f, UI.Measure(F.Cta, 17.0f, 700) + 44.0f);
		C->SetAlpha(A0 * Alpha);
		if (UI.Button("slide:" + F.Cta, {Lx + 230.0f, R.Y + 152.0f, Cw, 46.0f}, F.Cta, O) && Interactive)
		{
			switch (F.Action)
			{
			case 0:
				SetFilter(Filter::All, Now);
				SelectEvent(F.E.Id);
				S.ConfirmRegister = true;
				break;
			case 1: ShowSeries(F.SeriesId, Now); OpenPage(Page::Series, Now); break;
			case 2: SelectEvent(F.E.Id); break;
			default: ShowBoard(net::Board::NightShift, Now); OpenPage(Page::Leaderboards, Now); break;
			}
		}
	}
	(void)Net;
	C->Restore();
	C->SetAlpha(A0);
}

void RiverLine::Featured(const Rect& R, double Now)
{
	const std::vector<FeatureSlide>& All = FeatureSlides();
	if (All.empty())
	{
		return;
	}
	const int N = static_cast<int>(All.size());
	SlideShown = std::min(SlideShown, N - 1);
	const bool Hover = UI.Hover(R);
	auto Go = [&](int To) {
		if (To != SlideShown)
		{
			SlidePrev = SlideShown;
			SlideShown = (To % N + N) % N;
			SlideAt = Now;
		}
	};
	if (Now - SlideAt > SlideSeconds && !Hover)
	{
		Go(SlideShown + 1);
	}
	C->GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 16.0f, Rgba(0, 0, 0, 0.45f), 24.0f);
	C->PushClip(R);
	const float T = NetEase((Now - SlideAt) / 0.7);
	if (SlidePrev >= 0 && SlidePrev < N && T < 1.0f)
	{
		DrawSlide(All[static_cast<size_t>(SlidePrev)], R, Now, 1.0f - T, -T * 90.0f, false);
	}
	DrawSlide(All[static_cast<size_t>(SlideShown)], R, Now, T, (1.0f - T) * 90.0f, true);
	C->PopClip();
	C->StrokeRoundRect(R, 16.0f, Rgba(255, 255, 255, 0.08f), 1.0f);
	// Page dots, the active one filling up until the next slide.
	float Dx = R.X + 36.0f;
	const float Dy = R.Y + R.H - 12.0f;
	for (int I = 0; I < N; ++I)
	{
		const bool On = I == SlideShown;
		const float W = On ? 34.0f : 8.0f;
		const Ui::ClickState St = UI.Clickable("dot" + std::to_string(I), {Dx - 3.0f, Dy - 8.0f, W + 6.0f, 16.0f});
		if (St.Clicked)
		{
			Go(I);
		}
		UI.RRect({Dx, Dy - 3.0f, W, 6.0f}, 3.0f, Rgba(255, 255, 255, St.Hover ? 0.4f : 0.2f));
		if (On)
		{
			const float P = Nf(Clamp01((Now - SlideAt) / SlideSeconds));
			UI.RRect({Dx, Dy - 3.0f, std::max(6.0f, W * P), 6.0f}, 3.0f, Hex(All[static_cast<size_t>(I)].Col));
		}
		Dx += W + 8.0f;
	}
	// Arrows on hover.
	if (Hover)
	{
		for (int K = 0; K < 2; ++K)
		{
			const float Cx = R.X + R.W - 76.0f + Nf(K) * 44.0f;
			const float Cy = R.Y + 34.0f;
			const Ui::ClickState St = UI.Clickable(K == 0 ? "slideprev" : "slidenext", {Cx - 17.0f, Cy - 17.0f, 34.0f, 34.0f});
			C->FillCircle(Cx, Cy, 17.0f, Rgba(0, 0, 0, St.Hover ? 0.6f : 0.4f));
			C->StrokeEllipse(Cx, Cy, 17.0f, 17.0f, Rgba(255, 255, 255, 0.25f), 1.0f);
			NetChevron(*C, Cx, Cy, 14.0f, K == 0 ? 1 : 0, pal::Ink);
			if (St.Clicked)
			{
				Go(SlideShown + (K == 0 ? -1 : 1));
			}
		}
	}
}

void RiverLine::FilterChips(float X, float Y, float W, double Now, int Count)
{
	const char* Names[10] = {"All", "Playable", "Micro", "Low", "Mid & High", "Bounty", "Series", "Satellites", "Freerolls", "Running"};
	float Xx = X;
	for (int I = 0; I < 10; ++I)
	{
		const float Cw = UI.Measure(Names[I], 14.0f, 700) + (I == 1 ? 42.0f : 28.0f);
		const Rect R{Xx, Y, Cw, 32.0f};
		const Ui::ClickState St = UI.Clickable(std::string("chip") + Names[I], R);
		if (St.Clicked)
		{
			SetFilter(static_cast<Filter>(I), Now);
		}
		const bool On = FilterShown == static_cast<Filter>(I);
		if (On)
		{
			C->GlowRoundRect(R, 16.0f, NetA(pal::Accent, 0.3f), 10.0f);
			UI.RRect(R, 16.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(0x27d3c3), Hex(0x1fb3a5)));
		}
		else
		{
			UI.RRect(R, 16.0f, St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.02f), St.Hover ? Hex(0x2d4468) : pal::Line);
		}
		if (I == 1)
		{
			C->FillCircle(R.X + 16.0f, R.Y + 16.0f, 3.5f, On ? Hex(0x06201d) : pal::Green);
		}
		UI.Text(Names[I], R.X + R.W / 2.0f + (I == 1 ? 7.0f : 0.0f), R.Y + 16.5f, Ts(14.0f, 700, On ? Hex(0x06201d) : St.Hover ? pal::Ink : Hex(0xa9b5c8), Align::Center, Baseline::Middle));
		Xx += Cw + 8.0f;
	}
	UI.Text(Grouped(Count) + (Count == 1 ? " event" : " events"), X + W, Y + 21.0f, Ts(14.0f, 600, pal::Muted, Align::Right));
}

std::vector<RiverLine::ListRow> RiverLine::ScheduleRows() const
{
	const net::Network& Net = net::Shared();
	std::vector<ListRow> Rows;
	for (net::EventInstance& E : Net.Window(World - 16.0 * 60.0, World + 24.0 * 60.0))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, World);
		const bool Open = L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg;
		const bool InPlay = L.St == net::Status::Running || L.St == net::Status::FinalTable;
		const net::Tier Tr = net::TierOf(T.BuyInCents);
		std::string Lock;
		const bool Joinable = Net.Joinable(E, &Lock, S.Unlocks());
		const bool Pays = S.BankrollCents >= T.BuyInCents || S.Life.TicketsFor(Net.TicketOf(T.Id)) > 0;
		bool Keep = FilterShown == Filter::Running ? InPlay : Open;
		switch (FilterShown)
		{
		case Filter::Playable:
			Keep = Keep && Joinable && Pays && (L.St == net::Status::LateReg || L.StartsIn <= 60.0);
			break;
		case Filter::Micro: Keep = Keep && Tr == net::Tier::Micro; break;
		case Filter::Low: Keep = Keep && Tr == net::Tier::Low; break;
		case Filter::MidHigh: Keep = Keep && (Tr == net::Tier::Mid || Tr == net::Tier::High); break;
		case Filter::Bounty: Keep = Keep && (T.Fmt == net::Format::Bounty || T.Fmt == net::Format::Mystery); break;
		case Filter::Series: Keep = Keep && !T.Series.empty(); break;
		case Filter::Satellites: Keep = Keep && T.Fmt == net::Format::Satellite; break;
		case Filter::Freerolls: Keep = Keep && Tr == net::Tier::Freeroll; break;
		default: break;
		}
		if (Keep)
		{
			ListRow Row;
			Row.E = std::move(E);
			Row.L = L;
			Row.Joinable = Joinable;
			Row.Lock = Lock;
			Rows.push_back(std::move(Row));
		}
	}
	return Rows;
}

void RiverLine::Schedule(const Rect& R, const std::vector<ListRow>& Rows, double Now)
{
	// Header.
	UI.RRect({R.X, R.Y, R.W, 34.0f}, 8.0f, Hex(0x14223a));
	const std::pair<const char*, float> Heads[6] = {{"START", 20.0f}, {"TOURNAMENT", 130.0f}, {"BUY-IN", 530.0f}, {"PRIZE POOL", 610.0f}, {"ENTRIES", 770.0f}, {"STATUS", 884.0f}};
	for (const auto& Hd : Heads)
	{
		NetSpaced(*C, Hd.first, R.X + Hd.second, R.Y + 22.0f, 11.0f, 800, pal::Muted, 1.2f);
	}
	const Rect Area{R.X, R.Y + 42.0f, R.W, R.H - 42.0f};
	const float Pitch = 60.0f;
	const float Content = Nf(Rows.size()) * Pitch;
	const float MaxScroll = std::max(0.0f, Content - Area.H + 6.0f);
	if (UI.Hover(Area) && UI.Ptr.Wheel != 0.0f)
	{
		// The hosts send the wheel as the web does (positive: down, about 100 a notch): a row and a half a notch.
		ListScrollGoal += (UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * Pitch * 1.5f;
	}
	Listed.clear();
	int SelectedIndex = -1;
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		Listed.push_back(Rows[I].E.Id);
		SelectedIndex = Rows[I].E.Id == EventId ? static_cast<int>(I) : SelectedIndex;
	}
	if (ScrollToSelected && SelectedIndex >= 0)
	{
		const float Top = Nf(SelectedIndex) * Pitch;
		if (Top < ListScrollGoal || Top + Pitch > ListScrollGoal + Area.H)
		{
			ListScrollGoal = Top - Area.H * 0.4f;
		}
	}
	ScrollToSelected = false;
	ListScrollGoal = std::min(std::max(ListScrollGoal, 0.0f), MaxScroll);
	ListScroll += (ListScrollGoal - ListScroll) * NetFollow(Dt, 14.0);
	if (std::fabs(ListScrollGoal - ListScroll) < 0.3f)
	{
		ListScroll = ListScrollGoal;
	}
	C->PushClip(Area);
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * Pitch - ListScroll;
		if (Y + Pitch < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		ScheduleRow(Rows[I], {Area.X, Y, Area.W - 12.0f, Pitch - 6.0f}, Now, static_cast<int>(I) - static_cast<int>(ListScroll / Pitch));
	}
	C->PopClip();
	if (Rows.empty())
	{
		UI.Text("Nothing on the schedule for this filter right now.", Area.X + Area.W / 2.0f, Area.Y + 120.0f, Ts(17.0f, 600, pal::Muted, Align::Center));
		UI.Text("Try All, or check the Running tab.", Area.X + Area.W / 2.0f, Area.Y + 148.0f, Ts(15.0f, 500, pal::Dim, Align::Center));
	}
	// Scroll bar.
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * ListScroll / MaxScroll;
		UI.RRect({Area.X + Area.W - 5.0f, Area.Y, 4.0f, Area.H}, 2.0f, Rgba(255, 255, 255, 0.04f));
		UI.RRect({Area.X + Area.W - 5.0f, Ty, 4.0f, Th}, 2.0f, Rgba(255, 255, 255, 0.22f));
	}
}

void RiverLine::ScheduleRow(const ListRow& Row, const Rect& R, double Now, int Index)
{
	const net::Network& Net = net::Shared();
	const net::EventTemplate& T = Net.TemplateOf(Row.E);
	const net::LiveState& L = Row.L;
	const double Since = Now - std::max(FilterAt, PageAt) - 0.03 * static_cast<double>(std::min(std::max(Index, 0), 14));
	const float In = NetEase(Since / 0.4);
	const float A0 = C->GetAlpha();
	C->Save();
	C->SetAlpha(A0 * In);
	C->Translate((1.0f - In) * 34.0f, 0.0f);
	const Ui::ClickState St = UI.Clickable("ev:" + Row.E.Id, R);
	if (St.Clicked)
	{
		SelectEvent(Row.E.Id);
		ScrollToSelected = false;
	}
	const bool Sel = EventId == Row.E.Id;
	const Color Edge = NetEdge(Net, T);
	if (Sel)
	{
		C->GlowRoundRect(R, 10.0f, NetA(Edge, 0.22f), 12.0f);
		UI.RRect(R, 10.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(Hex(0x111c2e), Edge, 0.2f), Hex(0x111c2e)), NetA(Edge, 0.85f), 1.5f);
	}
	else
	{
		UI.RRect(R, 10.0f, St.Hover ? Hex(0x15253d) : Hex(0x101a2b), St.Hover ? Hex(0x2a3f60) : Rgba(255, 255, 255, 0.03f));
	}
	C->FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, Edge);
	const float Top = R.Y + 24.0f;
	const float Low = R.Y + 43.0f;
	const int Tickets = S.Life.TicketsFor(Net.TicketOf(T.Id));
	const bool Affordable = S.BankrollCents >= T.BuyInCents || Tickets > 0 || (stake::ActivePackage(S.Life.Stakes) && OnlineFunding(S, Row.E.Id).From == life::Paid::Package);
	const bool Dim = !Row.Joinable || !Affordable;
	// Start.
	UI.Text(net::TimeLabel(Row.E.Start), R.X + 20.0f, Top, Ts(16.0f, 700, pal::Ink));
	std::string When;
	Color WhenCol = pal::Muted;
	switch (L.St)
	{
	case net::Status::LateReg: When = "Level " + std::to_string(L.Level); WhenCol = Hex(0x7df0a0); break;
	case net::Status::Registering: When = "in " + net::Countdown(L.StartsIn); break;
	case net::Status::Announced: When = "opens " + net::Countdown(L.RegOpensIn); WhenCol = pal::Dim; break;
	case net::Status::Running: When = "Level " + std::to_string(L.Level); break;
	case net::Status::FinalTable: When = "Final table"; WhenCol = pal::Gold; break;
	default: break;
	}
	UI.Text(When, R.X + 20.0f, Low, Ts(12.0f, 600, WhenCol, Align::Left, Baseline::Alphabetic, false, 100.0f));
	// The event's emblem, its name and badges.
	{
		const float Ea = C->GetAlpha();
		C->SetAlpha(Ea * (Dim ? 0.6f : 1.0f));
		eventart::Emblem(*C, T, R.X + 148.0f, R.Y + R.H / 2.0f, 42.0f, Now);
		C->SetAlpha(Ea);
	}
	const float Nx = R.X + 180.0f;
	float Tw = UI.Text(T.Name, Nx, Top, Ts(17.0f, 700, Dim ? Hex(0xb4bfd0) : pal::Ink, Align::Left, Baseline::Alphabetic, false, 318.0f));
	if (T.Featured)
	{
		NetStar(*C, Nx + Tw + 12.0f, Top - 6.0f, 6.5f, pal::Gold);
	}
	float Bx = Nx;
	const float By = R.Y + 32.0f;
	if (!T.Series.empty())
	{
		const net::SeriesInfo* Sr = Net.FindSeries(T.Series);
		Bx += NetPill(*C, Sr ? Sr->Short : T.Series, Bx, By, Edge, true, 9.5f) + 5.0f;
	}
	if (T.Bracelet || T.Ring)
	{
		Bx += NetPill(*C, T.Bracelet ? "BRACELET" : "RING", Bx, By, Hex(0xf2c14e), true, 9.5f) + 5.0f;
	}
	if (T.Main)
	{
		Bx += NetPill(*C, "MAIN EVENT", Bx, By, pal::Gold, false, 9.5f) + 5.0f;
	}
	if (T.Speed == "Hyper" || T.Speed == "Turbo" || T.Speed == "Deep")
	{
		Bx += NetPill(*C, NetUpper(T.Speed), Bx, By, T.Speed == "Hyper" ? pal::Orange : T.Speed == "Turbo" ? Hex(0x4f9bff) : Hex(0x9b6bff), false, 9.5f) + 5.0f;
	}
	const char* FmtTag = T.Fmt == net::Format::Bounty ? "PKO" : T.Fmt == net::Format::Mystery ? "MYSTERY" : T.Fmt == net::Format::Satellite ? "SATELLITE" : T.Fmt == net::Format::Flip ? "FLIP & GO" : T.Fmt == net::Format::ReEntry ? "RE-ENTRY" : nullptr;
	if (FmtTag)
	{
		const Color Fc = T.Fmt == net::Format::Bounty ? pal::Orange : T.Fmt == net::Format::Mystery ? Hex(0xd46bff) : T.Fmt == net::Format::Satellite ? pal::Gold : T.Fmt == net::Format::Flip ? pal::Red : pal::Muted;
		Bx += NetPill(*C, FmtTag, Bx, By, Fc, false, 9.5f) + 5.0f;
	}
	if (T.TableSize == 6)
	{
		Bx += NetPill(*C, "6-MAX", Bx, By, pal::Muted, false, 9.5f) + 5.0f;
	}
	if (T.Omaha)
	{
		Bx += NetPill(*C, "PLO", Bx, By, Hex(0x4f9bff), false, 9.5f) + 5.0f;
	}
	if (Tickets > 0)
	{
		Bx += NetPill(*C, "TICKET", Bx, By, pal::Gold, true, 9.5f) + 5.0f;
	}
	if (!T.Seats.empty())
	{
		UI.Text("\xE2\x86\x92 " + T.Seats, Bx + 2.0f, R.Y + 45.0f, Ts(11.5f, 600, pal::Gold, Align::Left, Baseline::Alphabetic, false, Nx + 380.0f - Bx));
	}
	// Buy-in.
	UI.Text(net::BuyIn(T.BuyInCents), R.X + 585.0f, R.Y + 34.0f, Ts(17.0f, 800, T.BuyInCents == 0 ? pal::Green : Affordable ? pal::Gold : Hex(0x9c8752), Align::Right, Baseline::Alphabetic, true));
	// Prize pool.
	const float Px = R.X + 610.0f;
	const float Pw = UI.Text(NetMoney(L.Pool), Px, Top, Ts(17.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	if (L.Overlay)
	{
		NetPill(*C, "OVERLAY", Px, R.Y + 32.0f, pal::Red, true, 9.0f);
	}
	else if (T.GtdCents > 0 && L.Pool > T.GtdCents)
	{
		const double Up = static_cast<double>(L.Pool - T.GtdCents) / static_cast<double>(T.GtdCents) * 100.0;
		NetTriangle(*C, Px + 5.0f, R.Y + 40.0f, 8.0f, true, pal::Green);
		UI.Text(Fixed(Up, 0) + "% over GTD", Px + 13.0f, Low, Ts(11.5f, 700, pal::Green));
	}
	else if (T.GtdCents > 0)
	{
		UI.Text("GUARANTEED", Px, Low, Ts(10.5f, 700, pal::Dim));
	}
	(void)Pw;
	// Entries: count so far, and the share of the expected field.
	const float Ex = R.X + 770.0f;
	UI.Text(Grouped(L.Entries), Ex, Top, Ts(16.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	if (L.St == net::Status::Running || L.St == net::Status::FinalTable)
	{
		UI.Text(Grouped(L.Left) + " left", Ex, Low, Ts(11.5f, 600, pal::Muted));
	}
	else
	{
		const float Fill = Row.E.Entries > 0 ? std::min(1.0f, Nf(L.Entries) / Nf(Row.E.Entries)) : 0.0f;
		UI.RRect({Ex, R.Y + 36.0f, 86.0f, 4.0f}, 2.0f, Rgba(255, 255, 255, 0.07f));
		UI.RRect({Ex, R.Y + 36.0f, std::max(4.0f, 86.0f * Fill), 4.0f}, 2.0f, NetStatus(L.St));
	}
	// Status.
	const Color Sc = NetStatus(L.St);
	const Rect Sp{R.X + 880.0f, R.Y + 14.0f, 124.0f, 26.0f};
	UI.RRect(Sp, 13.0f, NetA(Sc, 0.12f), NetA(Sc, 0.45f));
	const float Pulse = L.St == net::Status::LateReg ? 0.55f + 0.45f * Nf(std::sin(Now * 5.0)) : 1.0f;
	C->FillCircle(Sp.X + 14.0f, Sp.Y + 13.0f, 4.0f, NetA(Sc, Pulse));
	if (L.St == net::Status::LateReg)
	{
		UI.Text("LATE", Sp.X + 24.0f, Sp.Y + 17.5f, Ts(11.0f, 800, Sc));
		UI.Text(NetHms(L.LateRegLeft), Sp.X + Sp.W - 11.0f, Sp.Y + 17.5f, Ts(12.0f, 700, Sc, Align::Right, Baseline::Alphabetic, true));
	}
	else
	{
		UI.Text(NetUpper(net::StatusName(L.St)), Sp.X + 24.0f, Sp.Y + 17.5f, Ts(11.0f, 800, Sc));
	}
	// Play or locked.
	const float Ix = R.X + R.W - 22.0f;
	if (!Row.Joinable)
	{
		NetLockIcon(*C, Ix - 8.0f, R.Y + R.H / 2.0f - 9.0f, 16.0f, pal::Dim);
	}
	else if (Affordable)
	{
		NetChevron(*C, Ix, R.Y + R.H / 2.0f, 14.0f, 0, St.Hover || Sel ? pal::Accent : NetA(pal::Accent, 0.55f));
	}
	C->Restore();
	C->SetAlpha(A0);
	(void)Tw;
}

void RiverLine::EventPanel(const Rect& R, double Now)
{
	const net::Network& Net = net::Shared();
	net::EventInstance E;
	Panel(R);
	if (!Net.FindInstance(EventId, E))
	{
		UI.Text("Select a tournament", R.X + R.W / 2.0f, R.Y + 200.0f, Ts(18.0f, 600, pal::Muted, Align::Center));
		return;
	}
	const net::EventTemplate& T = Net.TemplateOf(E);
	const net::LiveState L = Net.Live(E, World);
	const Color Edge = NetEdge(Net, T);
	const float In = NetEase((Now - EventAt) / 0.35);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * (0.3f + 0.7f * In));
	// Header band.
	C->FillRoundRect({R.X, R.Y, R.W, 168.0f}, 14.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + 168.0f}, Mix(Hex(0x122036), Edge, 0.3f), Hex(0x122036)));
	C->PushClip({R.X, R.Y, R.W, 168.0f});
	DrawSuit(*C, 3, R.X + R.W - 150.0f, R.Y - 20.0f + (1.0f - In) * 20.0f, 190.0f, NetA(Hex(0xffffff), 0.03f));
	C->PopClip();
	// The event's emblem (a series event's crest, dressed for what it means).
	eventart::Emblem(*C, T, R.X + R.W - 84.0f, R.Y + 88.0f + (1.0f - In) * 10.0f, 132.0f, Now);
	std::string Kicker;
	if (const net::SeriesInfo* Sr = Net.FindSeries(T.Series))
	{
		Kicker = NetUpper(Sr->Name.size() > 18 ? Sr->Short : Sr->Name) + " \xC2\xB7 EVENT #" + std::to_string(T.EventNo) + (T.Bracelet ? " \xC2\xB7 BRACELET" : T.Ring ? " \xC2\xB7 RING" : "");
	}
	else
	{
		Kicker = NetUpper(net::TierName(net::TierOf(T.BuyInCents))) + (net::TierOf(T.BuyInCents) == net::Tier::Freeroll ? "" : " STAKES");
	}
	Kicker += " \xC2\xB7 " + NetUpper(net::WeekdayName(net::DayOf(E.Start))) + " " + NetUpper(net::DateLabel(net::DayOf(E.Start)));
	NetSpaced(*C, Kicker, R.X + 26.0f, R.Y + 38.0f, 11.0f, 800, Edge, 1.6f);
	float Ty = NetParagraph(*C, T.Name, R.X + 26.0f, R.Y + 74.0f, R.W - 200.0f, 27.0f, 900, pal::Ink, 32.0f, 2);
	std::string Sub = std::string(T.Omaha ? "PL Omaha" : "NL Hold'em") + " \xC2\xB7 " + T.Speed + " \xC2\xB7 " + std::to_string(static_cast<int>(T.LevelMinutes)) + "-min levels \xC2\xB7 " + std::to_string(T.TableSize) + "-max";
	UI.Text(Sub, R.X + 26.0f, Ty + 2.0f, Ts(14.0f, 500, Hex(0xa9b5c8), Align::Left, Baseline::Alphabetic, false, R.W - 190.0f));
	NetPill(*C, NetFormat(T), R.X + 26.0f, R.Y + 138.0f, Edge, false, 10.0f);

	// Countdown and progress ring.
	const float Cy = R.Y + 186.0f;
	std::string Label;
	std::string Value;
	float Ring = 0.0f;
	std::string RingText;
	switch (L.St)
	{
	case net::Status::Announced:
		Label = "REGISTRATION OPENS IN";
		Value = NetHms(L.RegOpensIn);
		RingText = "SOON";
		break;
	case net::Status::Registering:
		Label = "STARTS IN";
		Value = NetHms(L.StartsIn);
		Ring = E.Entries > 0 ? Nf(L.Entries) / Nf(E.Entries) : 0.0f;
		RingText = Fixed(Ring * 100.0f, 0) + "%";
		break;
	case net::Status::LateReg:
		Label = "LATE REGISTRATION CLOSES IN";
		Value = NetHms(L.LateRegLeft);
		Ring = E.LateReg > 0.0 ? Nf(1.0 - L.LateRegLeft / E.LateReg) : 1.0f;
		RingText = "LVL " + std::to_string(L.Level);
		break;
	case net::Status::Running:
	case net::Status::FinalTable:
		Label = L.St == net::Status::FinalTable ? "FINAL TABLE \xC2\xB7 LEVEL " + std::to_string(L.Level) : "RUNNING \xC2\xB7 LEVEL " + std::to_string(L.Level);
		Value = Grouped(L.Left) + " left";
		Ring = Nf(L.Progress);
		RingText = Fixed(L.Progress * 100.0, 0) + "%";
		break;
	case net::Status::Finished:
		Label = "FINISHED";
		Value = net::TimeLabel(E.Start + E.Duration);
		Ring = 1.0f;
		RingText = "DONE";
		break;
	}
	NetSpaced(*C, Label, R.X + 26.0f, Cy + 22.0f, 11.0f, 700, pal::Muted, 1.4f);
	UI.Text(Value, R.X + 26.0f, Cy + 64.0f, Ts(38.0f, 700, L.St == net::Status::LateReg ? Hex(0x7df0a0) : pal::Ink, Align::Left, Baseline::Alphabetic, true));
	const float Rx = R.X + R.W - 66.0f;
	const float Ry = Cy + 40.0f;
	C->StrokeEllipse(Rx, Ry, 34.0f, 34.0f, Rgba(255, 255, 255, 0.07f), 6.0f);
	if (Ring > 0.001f)
	{
		C->StrokeArc(Rx, Ry, 34.0f, -Pi / 2.0f, -Pi / 2.0f + 2.0f * Pi * std::min(1.0f, Ring * In), NetStatus(L.St), 6.0f, true);
	}
	UI.Text(RingText, Rx, Ry + 1.0f, Ts(12.0f, 800, pal::Ink, Align::Center, Baseline::Middle));

	// Facts.
	const Chips Fee = T.JoinIndex >= 0 ? Lobby()[static_cast<size_t>(T.JoinIndex)].Spec.FeeCents : net::FeeOf(T.BuyInCents);
	const int Paid = std::max(1, static_cast<int>(std::round(static_cast<double>(E.Entries) * 0.15)));
	std::string Late = "Closed";
	if (L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg)
	{
		Late = "Until " + net::TimeLabel(E.Start + E.LateReg);
	}
	const std::pair<std::string, std::string> Facts[6] = {
		{"BUY-IN", T.BuyInCents == 0 ? std::string("Free") : Money(T.BuyInCents - Fee) + " + " + Money(Fee)},
		{"PRIZE POOL", NetMoney(L.Pool) + (T.GtdCents > 0 && L.Pool <= T.GtdCents ? " GTD" : "")},
		{"ENTRIES", Grouped(L.Entries) + (L.St == net::Status::Registering || L.St == net::Status::LateReg ? " of ~" + Grouped(E.Entries) : "")},
		{"PLACES PAID", Grouped(Paid)},
		{"STARTING STACK", Grouped(T.StartStack)},
		{"LATE REG", Late},
	};
	const float Fy = R.Y + 272.0f;
	for (int I = 0; I < 6; ++I)
	{
		const float X = R.X + 26.0f + Nf(I % 2) * (R.W - 52.0f) / 2.0f;
		const float Y = Fy + Nf(I / 2) * 54.0f;
		NetSpaced(*C, Facts[I].first, X, Y, 10.0f, 700, pal::Dim, 1.2f);
		UI.Text(Facts[I].second, X, Y + 24.0f, Ts(18.0f, 700, I == 1 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, (R.W - 52.0f) / 2.0f - 10.0f));
	}
	C->FillRect({R.X + 26.0f, R.Y + 426.0f, R.W - 52.0f, 1.0f}, pal::Line);

	// Tabs.
	const std::string Tabs[3] = {"Overview", "Payouts", L.St == net::Status::Finished ? "Final table" : "Players"};
	float Tx = R.X + 26.0f;
	for (int I = 0; I < 3; ++I)
	{
		const float W = UI.Measure(Tabs[I], 15.0f, 700);
		const Ui::ClickState St = UI.Clickable("dtab" + std::to_string(I), {Tx - 8.0f, R.Y + 436.0f, W + 16.0f, 36.0f});
		if (St.Clicked)
		{
			DetailTab = I;
		}
		const bool On = DetailTab == I;
		UI.Text(Tabs[I], Tx, R.Y + 460.0f, Ts(15.0f, 700, On ? pal::Ink : St.Hover ? Hex(0xc3cedf) : pal::Muted));
		if (On)
		{
			UI.RRect({Tx, R.Y + 468.0f, W, 3.0f}, 1.5f, Edge);
		}
		Tx += W + 26.0f;
	}
	const Rect Body{R.X + 26.0f, R.Y + 484.0f, R.W - 52.0f, R.H - 484.0f - 110.0f};
	if (DetailTab == 0)
	{
		std::string Blurb = T.Blurb;
		if (Blurb.empty())
		{
			Blurb = (T.BuyInCents == 0 ? std::string("A freeroll") : "A " + net::BuyIn(T.BuyInCents) + " " + NetLower(NetFormat(T))) + " with " +
				(T.GtdCents > 0 ? NetMoney(T.GtdCents) + " guaranteed" : std::string("a pool built by its entries")) + ". " + T.Speed + " structure, " +
				std::to_string(static_cast<int>(T.LevelMinutes)) + "-minute levels; about " + Grouped(E.Entries) + " players expected.";
		}
		float Y = NetParagraph(*C, Blurb, Body.X, Body.Y + 20.0f, Body.W, 16.0f, 500, Hex(0xc9d3e2), 23.0f, 3) + 4.0f;
		const int LateLevels = static_cast<int>(std::ceil(E.LateReg / std::max(1.0, T.LevelMinutes)));
		const std::pair<std::string, std::string> Structure[3] = {
			{"Blinds", "Level 1: " + std::string(T.StartStack > 10000 ? "100/200" : "50/100") + " \xC2\xB7 up every " + std::to_string(static_cast<int>(T.LevelMinutes)) + " min"},
			{"Late registration", net::Countdown(E.LateReg) + " (to level " + std::to_string(LateLevels + 1) + ")"},
			{"Estimated finish", net::TimeLabel(E.Start + E.Duration) + " \xC2\xB7 " + net::Countdown(E.Duration) + " of poker"},
		};
		for (const auto& Row : Structure)
		{
			UI.Text(Row.first, Body.X, Y, Ts(13.0f, 600, pal::Muted));
			UI.Text(Row.second, Body.X + Body.W, Y, Ts(13.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, false, Body.W - 130.0f));
			Y += 24.0f;
		}
		if (!T.Seats.empty())
		{
			Y += 8.0f;
			UI.RRect({Body.X, Y, Body.W, 40.0f}, 8.0f, Rgba(242, 193, 78, 0.08f), NetA(pal::Gold, 0.35f));
			UI.Text("Prizes: " + T.Seats, Body.X + 14.0f, Y + 26.0f, Ts(14.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, false, Body.W - 28.0f));
			Y += 50.0f;
		}
		std::string Lock;
		if (!Net.Joinable(E, &Lock, S.Unlocks()))
		{
			Y += 8.0f;
			UI.RRect({Body.X, Y, Body.W, 54.0f}, 8.0f, Rgba(255, 255, 255, 0.03f), pal::Line);
			NetLockIcon(*C, Body.X + 14.0f, Y + 16.0f, 22.0f, pal::Muted);
			UI.Text("Locked", Body.X + 48.0f, Y + 23.0f, Ts(14.0f, 800, pal::Ink));
			UI.Text(Lock, Body.X + 48.0f, Y + 42.0f, Ts(13.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, Body.W - 60.0f));
		}
	}
	else if (DetailTab == 1)
	{
		const std::vector<Chips> Pay = Net.Payouts(E);
		const float Top = Pay.empty() ? 1.0f : Nf(Pay.front());
		for (size_t I = 0; I < Pay.size() && I < 8; ++I)
		{
			const float Y = Body.Y + 12.0f + Nf(I) * 27.0f;
			const float Bw = (Body.W - 170.0f) * Nf(Pay[I]) / Top * In;
			UI.Text(Ordinal(static_cast<int>(I) + 1), Body.X, Y + 16.0f, Ts(14.0f, 700, I == 0 ? pal::Gold : pal::Muted));
			UI.RRect({Body.X + 50.0f, Y + 6.0f, std::max(3.0f, Bw), 12.0f}, 3.0f, Paint::Linear({Body.X + 50.0f, 0.0f}, {Body.X + 50.0f + Body.W - 170.0f, 0.0f}, NetA(Edge, 0.9f), NetA(Edge, 0.35f)));
			UI.Text(Money(Pay[I]), Body.X + Body.W, Y + 16.0f, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		}
		UI.Text(Grouped(static_cast<int64_t>(Pay.size())) + " places paid \xC2\xB7 min cash " + (Pay.empty() ? std::string("-") : Money(Pay.back())), Body.X, Body.Y + Body.H - 4.0f, Ts(13.0f, 500, pal::Muted));
	}
	else if (L.St != net::Status::Finished)
	{
		// Who's in it: the regulars the world has registered, the best known first.
		std::vector<int> Who = S.Living().Registered(E.Id);
		const world::World& W = S.Living();
		std::stable_sort(Who.begin(), Who.end(), [&](int A, int B) { return W.Get(A)->RepOf(world::Rep::Overall) > W.Get(B)->RepOf(world::Rep::Overall); });
		UI.Text(Who.empty() ? std::string("No regulars registered yet") : "Regulars registered \xC2\xB7 " + std::to_string(Who.size()), Body.X, Body.Y + 18.0f, Ts(13.0f, 600, pal::Muted));
		for (size_t I = 0; I < Who.size() && I < 8; ++I)
		{
			const world::Npc* N = W.Get(Who[I]);
			const float Y = Body.Y + 30.0f + Nf(I) * 29.0f;
			const AvatarSpec Pic = NetAvatar(*C, Body.X + 12.0f, Y + 13.0f, 11.0f, N->Name, Color{0.0f, 0.0f, 0.0f, 0.0f});
			PlayerName(Who[I], Body.X + 32.0f, Y + 18.0f, 14.0f, Body.W - 230.0f, true, NetBadgeFrame(Pic));
			// A name nobody has seen before: when they joined.
			const bool Fresh = N->Came != world::Arrival::None && net::DayOf(World) - N->Arrived <= 30;
			UI.Text(Fresh ? std::string("Joined ") + net::DateLabel(N->Arrived) : std::string(world::IdentityName(N->Is)), Body.X + Body.W, Y + 18.0f,
				Ts(12.0f, 600, Fresh ? pal::Accent : pal::Muted, Align::Right, Baseline::Alphabetic, false, 150.0f));
		}
	}
	else
	{
		const net::EventResult& Res = Net.Result(E);
		const bool Finished = L.St == net::Status::Finished;
		UI.Text(Finished ? "Final table results" : L.St == net::Status::Registering || L.St == net::Status::LateReg || L.St == net::Status::Announced ? "Regulars registered" : "At the top of the counts",
			Body.X, Body.Y + 18.0f, Ts(13.0f, 600, pal::Muted));
		for (size_t I = 0; I < Res.FinalTable.size() && I < 8; ++I)
		{
			const net::Placing& P = Res.FinalTable[I];
			const float Y = Body.Y + 30.0f + Nf(I) * 29.0f;
			if (Finished)
			{
				UI.Text(std::to_string(P.Place), Body.X + 12.0f, Y + 18.0f, Ts(14.0f, 800, P.Place == 1 ? pal::Gold : pal::Muted, Align::Center));
			}
			const std::string Nm = PlacingName(P);
			const AvatarSpec Pic = NetAvatar(*C, Body.X + 40.0f, Y + 13.0f, 11.0f, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, P.Player == -1);
			PlayerName(P, Body.X + 60.0f, Y + 18.0f, 14.0f, Body.W - 170.0f, true, NetBadgeFrame(Pic));
			if (Finished)
			{
				UI.Text(Money(P.Prize), Body.X + Body.W, Y + 18.0f, Ts(14.0f, 700, P.Place == 1 ? pal::Gold : pal::Ink, Align::Right, Baseline::Alphabetic, true));
			}
		}
	}
	C->SetAlpha(A0);
	// Who pays, and selling pieces of it (next_segment_spec §2.12): the package's line, or the Sell action while it's the
	// player's own money (the Backing page opens on this event, in 1% pieces). Not while the register button confirms.
	const bool Upcoming = L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg;
	if (Upcoming && !S.IsPlaying(E.Id) && !S.ConfirmRegister && T.BuyInCents > 0)
	{
		const life::Funding F = OnlineFunding(S, E.Id);
		const float Ly = R.Y + R.H - 106.0f;
		float Right = R.X + R.W - 26.0f;
		std::string Lock;
		if (F.From == life::Paid::Own && Net.Joinable(E, &Lock, S.Unlocks()))
		{
			const int Asked = stake::AskedOn(S.Life.Stakes, E.Id);
			const int Filled = stake::FilledOn(S.Life.Stakes, E.Id, World);
			const std::string Sell = Filled > 0 ? NetPermille(Filled) + " sold  \xE2\x80\xBA" : Asked > 0 ? "Selling " + NetPermille(Asked) + "  \xE2\x80\xBA" : std::string("Sell action  \xE2\x80\xBA");
			const float Sw = UI.Measure(Sell, 12.5f, 800) + 20.0f;
			const Rect Sr{Right - Sw, Ly - 15.0f, Sw, 22.0f};
			const Ui::ClickState Sc = UI.Clickable("osell" + E.Id, Sr);
			UI.RRect(Sr, 8.0f, Sc.Hover ? NetA(pal::Accent, 0.2f) : NetA(pal::Accent, 0.08f), NetA(pal::Accent, Sc.Hover ? 0.8f : 0.45f));
			UI.Text(Sell, Sr.X + Sw / 2.0f, Sr.Y + Sr.H / 2.0f + 1.0f, Ts(12.5f, 800, Sc.Hover ? pal::Ink : pal::Accent, Align::Center, Baseline::Middle));
			if (Sc.Clicked)
			{
				backing_detail::Preselect(UI, E.Id);
				OpenPage(Page::Backing, Now);
			}
			Right = Sr.X - 10.0f;
		}
		const std::string Line = live::FundingLine(F, NetShortName(T.Name));
		if (!Line.empty())
		{
			UI.Text(Line, R.X + 26.0f, Ly, Ts(12.5f, 700, Hex(0xf2a93b), Align::Left, Baseline::Alphabetic, false, Right - R.X - 26.0f));
		}
	}
	RegisterBlock(E, L, {R.X + 26.0f, R.Y + R.H - 96.0f, R.W - 52.0f, 70.0f}, Now);
}

void RiverLine::RegisterBlock(const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now)
{
	const net::Network& Net = net::Shared();
	const net::EventTemplate& T = Net.TemplateOf(E);
	std::string Lock;
	const bool Joinable = Net.Joinable(E, &Lock, S.Unlocks());
	const int Tickets = S.Life.TicketsFor(Net.TicketOf(T.Id));
	// A package's roll pays for what's in its range (next_segment_spec §2.7, S5): the bankroll needn't cover it.
	const life::Funding Fund = OnlineFunding(S, E.Id);
	const bool RollPays = Fund.From == life::Paid::Package;
	const bool Affordable = S.BankrollCents >= T.BuyInCents || Tickets > 0 || RollPays;
	const bool Open = L.St == net::Status::LateReg || (L.St == net::Status::Registering && L.StartsIn <= 60.0);
	ButtonOpts O;
	O.Size = 20.0f;
	auto Disabled = [&](const std::string& Label, const std::string& Sub) {
		O.Kind = ButtonKind::Secondary;
		O.Enabled = false;
		O.Sub = Sub;
		UI.Button("regx", R, Label, O);
	};
	if (S.IsPlaying(E.Id))
	{
		// Seated in it: the button goes to its table.
		int Seat = 0;
		for (int I = 0; I < S.TableCount(); ++I)
		{
			Seat = S.Glance(I).EventId == E.Id ? I : Seat;
		}
		const TableGlance G = S.Glance(Seat);
		O.Kind = G.YourTurn ? ButtonKind::Gold : ButtonKind::Primary;
		O.Sub = G.YourTurn ? std::string("It's your turn") : "You're in \xC2\xB7 " + Grouped(G.Rank) + " of " + Grouped(G.Remaining);
		if (UI.Button("regopen", R, "Open table", O))
		{
			S.ShowTables();
			S.FocusTable(Seat);
		}
		return;
	}
	if (L.St == net::Status::Finished)
	{
		const net::EventResult& Res = Net.Result(E);
		const net::Placing* W = Res.FinalTable.empty() ? nullptr : &Res.FinalTable.front();
		UI.RRect(R, 10.0f, Rgba(242, 193, 78, 0.07f), NetA(pal::Gold, 0.3f));
		NetTrophy(*C, R.X + 34.0f, R.Y + 16.0f, 40.0f, Hex(0xffe08a), Hex(0xc9962b));
		UI.Text("Winner", R.X + 68.0f, R.Y + 28.0f, Ts(12.0f, 700, pal::Muted));
		if (W)
		{
			UI.Text(PlacingName(*W), R.X + 68.0f, R.Y + 52.0f, Ts(19.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 200.0f));
			UI.Text(Money(W->Prize), R.X + R.W - 18.0f, R.Y + 44.0f, Ts(22.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
		}
	}
	else if (L.St == net::Status::Running || L.St == net::Status::FinalTable)
	{
		Disabled("Registration closed", "Level " + std::to_string(L.Level) + " \xC2\xB7 " + Grouped(L.Left) + " players left");
	}
	else if (!Joinable)
	{
		Disabled("Locked", Lock);
	}
	else if (S.Restricted())
	{
		Disabled("Account under review", "RiverLine security \xC2\xB7 " + net::Countdown(S.Life.BannedUntil - World) + " left");
	}
	else if (S.TimeSkip.Active)
	{
		Disabled("You're away", S.TimeSkip.Label);
	}
	else if (!Open)
	{
		Disabled("Opens in " + net::Countdown(L.StartsIn - 60.0), "Seats open an hour before the start");
	}
	else if (!Affordable)
	{
		Disabled("Insufficient funds", "Balance " + Money(S.BankrollCents) + " \xC2\xB7 buy-in " + net::BuyIn(T.BuyInCents));
	}
	else if (S.TableCount() >= S.MaxTables())
	{
		Disabled("Table limit reached", S.MaxTables() < ss::Session::TableLimit ? "Too tired for more than " + std::to_string(S.MaxTables()) + " tables" : "Four tables is as many as you can follow");
	}
	else if (!S.ConfirmRegister)
	{
		O.Kind = ButtonKind::Primary;
		O.Sub = L.St == net::Status::LateReg ? "Late registration \xC2\xB7 " + NetHms(L.LateRegLeft) + " left" : "Starts at " + net::TimeLabel(E.Start);
		const float Glow = 0.5f + 0.5f * Nf(std::sin(Now * 2.4));
		C->GlowRoundRect(R, 10.0f, NetA(pal::Accent, 0.18f + 0.14f * Glow), 18.0f);
		if (Tickets > 0)
		{
			O.Kind = ButtonKind::Gold;
			O.Sub = "Pays with your ticket (" + std::to_string(Tickets) + " held)";
		}
		else if (RollPays)
		{
			O.Sub = Fund.Who + "'s roll pays";
		}
		if (UI.Button("reg", R, Tickets > 0 ? std::string("Register with ticket") : "Register \xC2\xB7 " + net::BuyIn(T.BuyInCents), O))
		{
			S.ConfirmRegister = true;
		}
	}
	else
	{
		UI.Text(Tickets > 0 ? std::string("One ticket will be used")
				: RollPays  ? "The roll pays: your balance stays " + Money(S.BankrollCents)
							: "Balance after: " + Money(S.BankrollCents - T.BuyInCents),
			R.X, R.Y - 10.0f, Ts(14.0f, 600, pal::Muted));
		ButtonOpts Ok;
		Ok.Kind = ButtonKind::Gold;
		Ok.Size = 20.0f;
		const float Hw = (R.W - 12.0f) / 2.0f;
		if (UI.Button("regok", {R.X, R.Y, Hw, R.H}, "Confirm", Ok))
		{
			S.ConfirmRegister = false;
			S.RegisterEvent(Net.Listing(E, nullptr, S.Unlocks()));
		}
		ButtonOpts No;
		No.Kind = ButtonKind::Ghost;
		No.Size = 20.0f;
		if (UI.Button("regno", {R.X + Hw + 12.0f, R.Y, Hw, R.H}, "Cancel", No))
		{
			S.ConfirmRegister = false;
		}
	}
}

void RiverLine::Ticker(const Rect& R, double Now)
{
	const net::Network& Net = net::Shared();
	// The feed: headlines, then the network's own numbers.
	struct Item
	{
		std::string Text;
		Color Col;
	};
	std::vector<Item> Items;
	const double Hour = std::fmod(World, net::MinutesPerDay) / 60.0;
	const int Online = static_cast<int>(318000.0 + 94000.0 * std::cos((Hour - 21.0) / 24.0 * 2.0 * 3.14159265) + 900.0 * std::sin(World * 0.7));
	Items.push_back({Grouped(Online) + " players online", pal::Green});
	for (const net::NewsItem& It : NewsFeed())
	{
		if (Items.size() > 9)
		{
			break;
		}
		Items.push_back({It.Title + (It.Amount > 0 ? " (" + Money(It.Amount) + ")" : std::string()), NetKind(It.Kind)});
	}
	if (const net::SeriesInfo* Sr = Net.CurrentSeries(World))
	{
		const int Day = net::DayOf(World);
		Items.push_back({Day >= Sr->FirstDay ? Sr->Name + ": day " + std::to_string(Day - Sr->FirstDay + 1) + " of " + std::to_string(Sr->LastDay - Sr->FirstDay + 1)
												: Sr->Name + " starts " + net::DateLabel(Sr->FirstDay),
			Hex(Sr->Color)});
	}
	float Total = 0.0f;
	for (const Item& It : Items)
	{
		Total += UI.Measure(It.Text, 13.0f, 600) + 62.0f;
	}
	// The label, then the scrolling lane.
	UI.RRect({R.X, R.Y + 9.0f, 52.0f, R.H - 18.0f}, 5.0f, pal::Red);
	C->FillCircle(R.X + 11.0f, R.Y + R.H / 2.0f, 3.0f, Rgba(255, 255, 255, 0.55f + 0.45f * Nf(std::sin(Now * 4.0))));
	UI.Text("LIVE", R.X + 18.0f, R.Y + R.H / 2.0f + 1.0f, Ts(11.0f, 900, Hex(0xffffff), Align::Left, Baseline::Middle));
	const Rect Lane{R.X + 60.0f, R.Y, R.W - 60.0f, R.H};
	C->PushClip(Lane);
	float X = Lane.X + 20.0f - Nf(std::fmod(Now * 60.0, static_cast<double>(Total)));
	for (int Rep = 0; Rep < 3 && X < Lane.X + Lane.W; ++Rep)
	{
		for (const Item& It : Items)
		{
			C->FillCircle(X, R.Y + R.H / 2.0f, 3.0f, It.Col);
			X += 12.0f;
			X += UI.Text(It.Text, X, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 600, Hex(0xb9c4d6), Align::Left, Baseline::Middle)) + 50.0f;
		}
	}
	C->PopClip();
	const Color Bar = Hex(0x080e18);
	C->FillRect({Lane.X, R.Y + 1.0f, 30.0f, R.H - 1.0f}, Paint::Linear({Lane.X, 0.0f}, {Lane.X + 30.0f, 0.0f}, Bar, NetA(Bar, 0.0f)));
	C->FillRect({Lane.X + Lane.W - 30.0f, R.Y + 1.0f, 30.0f, R.H - 1.0f}, Paint::Linear({Lane.X + Lane.W - 30.0f, 0.0f}, {Lane.X + Lane.W, 0.0f}, NetA(Bar, 0.0f), Bar));
}

// ------------------------------------------------------------------ pages

void RiverLine::LobbyPages(double Now)
{
	const float In = NetEase((Now - PageAt) / 0.4);
	const float A0 = C->GetAlpha();
	// A player card is up: the page underneath takes no input.
	const bool Card = CardOpen();
	const Pointer Real = UI.Ptr;
	if (Card)
	{
		UI.Ptr.Pressed = false;
		UI.Ptr.Released = false;
		UI.Ptr.Wheel = 0.0f;
		UI.Ptr.X = -1.0f;
		UI.Ptr.Y = -1.0f;
	}
	C->Save();
	C->SetAlpha(A0 * In);
	C->Translate(0.0f, (1.0f - In) * 16.0f);
	switch (PageShown)
	{
	case Page::Lobby: NetLobby(Now); break;
	case Page::Series: SeriesPage(Now); break;
	case Page::Leaderboards: BoardsPage(Now); break;
	case Page::News: NewsPage(Now); break;
	case Page::Career: CareerPage(Now); break;
	case Page::Embercrest: EmbercrestPage(Now); break;
	case Page::Backing: BackingPage(Now); break; // RiverLineBacking.cpp (core-stake's page; the tab is the nav's)
	}
	C->Restore();
	C->SetAlpha(A0);
	if (Card)
	{
		UI.Ptr = Real;
		PlayerCard(Now);
	}
}

// ------------------------------------------------------------------ the Embercrest

namespace rlnet_detail
{
/** An event's name without its price ("Embercrest Nightly"): the card prints the buy-in on its own. */
std::string LiveTitle(const live::EventTemplate& T)
{
	const std::string Name = T.Name;
	const size_t At = Name.rfind(" $");
	return At == std::string::npos ? Name : Name.substr(0, At);
}

/**
 * The Embercrest page's LEADERS (next_segment_spec §5.3, §5.6): the player's Players Club card (220 x 138) with where
 * they stand in it, then the month's top ten in the room's points, each with the one glyph the room would show (Room
 * scope), the player's row in ember. Recognition only: nothing here is a prize (Q24).
 */
void LiveLeaders(Canvas& Cv, Ui& UI, const RiverLine* RL, const Session& S, const Rect& R, double World, double Now, double Since)
{
	const Color Ember = Hex(EcEmberHot);
	const world::World& W = S.Living();
	const int Today = net::DayOf(World);
	const life::State& L = S.Life;
	// The card, as the desk prints it.
	const badge::Badge* Club = S.HeroBadges().Find(badge::Kind::Club);
	const int Tier = Club ? Club->Tier : 0;
	const int Visits = W.HeroRoom().VisitsIn(Today);
	ui::badgeart::ClubCardInfo Info;
	Info.Name = S.Person.Created ? S.Person.FullName() : S.HeroName;
	Info.ScreenName = S.HeroName;
	Info.Country = S.Person.Country;
	Info.Tier = Tier;
	Info.Visits = Visits;
	Info.Wins = L.LiveWins;
	Info.SinceDay = Today;
	for (const life::LiveDay& D : L.LiveDays)
	{
		if (D.Embercrest > 0)
		{
			Info.SinceDay = std::min(Info.SinceDay, D.Day);
		}
	}
	Info.New = Tier == 0 && Visits == 0;
	const float Ci = NetEase(Since / 0.5);
	const Rect Card{R.X + 24.0f, R.Y + 58.0f + (1.0f - Ci) * 8.0f, 220.0f, 138.0f};
	badgeart::ClubCard(Cv, Card, Info, Now);
	// Where they stand: the tier in words, the visits that count, what the next card takes (§5.3: 6, 18, 36 in 90 days).
	const float Tx = R.X + 264.0f;
	const float Tw = R.X + R.W - 24.0f - Tx;
	NetSpaced(Cv, "PLAYERS CLUB", Tx, R.Y + 78.0f, 10.0f, 800, Ember, 1.6f);
	const char* Words = Tier >= 3 ? "Crest" : Tier == 2 ? "Ember" : Tier == 1 ? "Regular" : "Member";
	UI.Text(Words, Tx, R.Y + 106.0f, Ts(22.0f, 900, Tier > 0 ? pal::Ink : pal::Muted));
	UI.Text(std::to_string(Visits) + (Visits == 1 ? " event" : " events") + " in 90 days", Tx, R.Y + 130.0f, Ts(13.0f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, Tw));
	const int Need = Tier >= 3 ? 0 : Tier == 2 ? badge::ClubCrest : Tier == 1 ? badge::ClubEmber : badge::ClubRegular;
	const char* NextWord = Tier == 2 ? "Crest" : Tier == 1 ? "Ember" : "Regular";
	const std::string Next = Need == 0 ? std::string("The top card in the room.") : std::to_string(std::max(0, Need - Visits)) + " more for " + NextWord + ".";
	UI.Text(Next, Tx, R.Y + 150.0f, Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Tw));
	if (L.LiveWins > 0)
	{
		UI.Text("On the wall " + (L.LiveWins == 1 ? std::string("once") : std::to_string(L.LiveWins) + " times"), Tx, R.Y + 170.0f, Ts(13.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, false, Tw));
	}

	// The month's leaders.
	int Year = 0;
	int Month = 0;
	int Date = 0;
	world::CivilDate(Today, Year, Month, Date);
	static const char* Months[12] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
	const float Ly = R.Y + 236.0f;
	Cv.FillRoundRect({R.X + 24.0f, Ly - 11.0f, 3.0f, 13.0f}, 1.5f, Ember);
	NetSpaced(Cv, std::string(Months[(Month + 11) % 12]) + " LEADERS", R.X + 35.0f, Ly, 12.0f, 800, pal::Muted, 1.6f);
	UI.Text("points", R.X + R.W - 24.0f, Ly, Ts(12.0f, 600, pal::Dim, Align::Right));
	const std::vector<std::pair<int, double>> Rows = W.RoomLeaders("embercrest", world::MonthKeyOf(Today), 10);
	if (Rows.empty())
	{
		NetParagraph(Cv, "Nobody's on the board yet this month. Points come from cashes in the room's own events.", R.X + 24.0f, Ly + 34.0f, R.W - 48.0f, 14.0f, 500, pal::Muted, 20.0f, 3);
	}
	for (size_t K = 0; K < Rows.size(); ++K)
	{
		const int Who = Rows[K].first;
		const bool Hero = Who == -1;
		const world::Npc* N = Hero ? nullptr : W.Get(Who);
		const std::string Name = Hero ? S.HeroName : N ? N->Name : std::string();
		const float Ri = NetEase((Since - 0.04 * static_cast<double>(K)) / 0.4);
		const float A0 = Cv.GetAlpha();
		Cv.SetAlpha(A0 * Ri);
		const Rect Row{R.X + 12.0f, Ly + 14.0f + Nf(K) * 40.0f, R.W - 24.0f, 36.0f};
		UI.RRect(Row, 8.0f, Hero ? NetA(Ember, 0.14f) : (K % 2 == 0 ? Hex(0x101a2b) : Hex(0x0e1726)), Hero ? NetA(Ember, 0.6f) : Rgba(255, 255, 255, 0.02f));
		UI.Text(std::to_string(K + 1), Row.X + 34.0f, Row.Y + 24.0f, Ts(15.0f, 900, K == 0 ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		// The one glyph the room shows by a name (Room scope: the wall, the club, a ring...).
		float Nx = Row.X + 48.0f;
		if (const badge::Badge* B = badge::Top(NetBadgesOf(S, Who), badge::Room, badge::FrameNone))
		{
			badgeart::Draw(Cv, *B, Nx + 9.0f, Row.Y + 18.0f, 18.0f, Now);
			if (UI.Hover({Nx - 2.0f, Row.Y + 7.0f, 22.0f, 22.0f}))
			{
				NetTip& Tip = NetTipOf(RL);
				Tip.Up = true;
				Tip.B = *B;
				Tip.Npc = Who;
				Tip.X = Nx + 9.0f;
				Tip.Y = Row.Y + 27.0f;
			}
			Nx += 26.0f;
		}
		UI.Text(Name, Nx, Row.Y + 24.0f, Ts(15.0f, Hero ? 900 : 700, Hero ? Ember : pal::Ink, Align::Left, Baseline::Alphabetic, false, Row.X + Row.W - 90.0f - Nx));
		UI.Text(Grouped(static_cast<int64_t>(std::llround(Rows[K].second))), Row.X + Row.W - 14.0f, Row.Y + 24.0f, Ts(15.0f, 800, Hero ? Ember : pal::Ink, Align::Right, Baseline::Alphabetic, true));
		Cv.SetAlpha(A0);
	}
	NetParagraph(Cv, "The room's own board: points from the Embercrest's events this month, the same race the side screens show between levels. No prize, just the room's respect.",
		R.X + 24.0f, R.Y + R.H - 66.0f, R.W - 48.0f, 12.5f, 500, pal::Dim, 17.0f, 3);
}

/**
 * Career's YOUR BADGES band (next_segment_spec §5.6), from y 400 to 508: the heading and the badges held beside it,
 * the sponsors' word for the player at the right (§3.3: the tier's words, never its thresholds), then up to three
 * 300 x 84 cards for the badges within reach (badge::Next, through Session::BadgeProgress), each an honest count.
 */
void CareerBadges(Canvas& Cv, Ui& UI, const RiverLine* RL, const Session& S, double Now, float In)
{
	const float Top = 412.0f;
	Cv.FillRoundRect({24.0f, Top - 11.0f, 3.0f, 13.0f}, 1.5f, pal::Gold);
	const float Hw = NetSpaced(Cv, "YOUR BADGES", 35.0f, Top, 12.0f, 800, pal::Muted, 1.6f) + 11.0f;
	// The sponsors' word (right-aligned): "SPONSORS  A face in the room".
	const std::string Words = sponsor::StandingWords(S.SponsorTier());
	const float Ww = UI.Text(Words, 1076.0f, Top, Ts(13.5f, 800, S.SponsorTier() > 0 ? pal::Ink : pal::Muted, Align::Right));
	const float Sw = NetSpaced(Cv, "SPONSORS", 1076.0f - Ww - 10.0f, Top, 10.5f, 800, pal::Dim, 1.4f, Align::Right) + Ww + 10.0f;
	// What's held, between the heading and the sponsors' word.
	const float Rx = 24.0f + Hw + 14.0f;
	const float RailMax = 1076.0f - Sw - 24.0f - Rx;
	const std::vector<badge::Badge> Held = NetCardBadges(S.HeroBadges(), 6);
	if (Held.empty())
	{
		UI.Text("None yet. The room, the site and the circuit notice before you do.", Rx, Top, Ts(12.5f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, RailMax));
	}
	else
	{
		NetBadgeRail(Cv, UI, RL, Held, -1, Rx, Top - 4.0f, 22.0f, 3, RailMax, Now);
	}
	// Within reach.
	const std::vector<badge::Progress> Next = S.BadgeProgress(3);
	for (size_t K = 0; K < Next.size() && K < 3; ++K)
	{
		const badge::Progress& P = Next[K];
		const Rect R{24.0f + Nf(K) * 312.0f, 424.0f, 300.0f, 84.0f};
		Cv.GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 12.0f, Rgba(0, 0, 0, 0.3f), 14.0f);
		UI.RRect(R, 12.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x122036), Hex(0x0d1729)), pal::Line);
		badge::Badge B;
		B.K = P.K;
		B.Tier = P.Tier;
		badgeart::Draw(Cv, B, R.X + 28.0f, R.Y + 32.0f, 30.0f, Now);
		const std::string Label = badge::Label(B);
		const float Lw = NetSpaced(Cv, Label.empty() ? std::string("NEXT") : Label, R.X + 52.0f, R.Y + 23.0f, 10.0f, 800, pal::Gold, 1.3f);
		// "Ember · 12 of 18 events in 90 days" under EMBER reads "12 of 18 events in 90 days": the label says the name.
		std::string Line = P.Line;
		const size_t Dot = Line.find(" \xC2\xB7 ");
		if (Dot != std::string::npos && !Label.empty() && NetUpper(Line.substr(0, Dot)) == Label.substr(0, std::min(Label.size(), Dot)))
		{
			Line = Line.substr(Dot + 4);
			if (!Line.empty() && Line[0] >= 'a' && Line[0] <= 'z')
			{
				Line[0] = static_cast<char>(Line[0] - 'a' + 'A');
			}
		}
		// The count, then what else the badge reads (the ROI, the days clear of the day job) on up to two lines, whole:
		// "30 of 30 days clear" is the part a ShiftLink shift resets (§9.1), so it never goes under an ellipsis.
		NetParagraph(Cv, Line, R.X + 52.0f, R.Y + 41.0f, R.W - 66.0f, 12.5f, 600, Hex(0xc3cedf), 16.0f, P.Detail.empty() ? 2 : 1);
		if (!P.Detail.empty())
		{
			NetParagraph(Cv, P.Detail, R.X + 52.0f, R.Y + 57.0f, R.W - 66.0f, 12.0f, 500, pal::Muted, 14.0f, 2);
		}
		// The bar rides the label's line, out to the card's edge.
		const float Frac = P.Need > 0.0 ? Nf(Clamp01(P.Have / P.Need)) : 0.0f;
		const float Bx = R.X + 52.0f + Lw + 10.0f;
		const Rect Bar{Bx, R.Y + 17.0f, std::max(40.0f, R.X + R.W - 14.0f - Bx), 4.0f};
		UI.RRect(Bar, 2.0f, Rgba(255, 255, 255, 0.07f));
		if (Frac > 0.0f)
		{
			UI.RRect({Bar.X, Bar.Y, std::max(4.0f, Bar.W * Frac * In), Bar.H}, 2.0f, pal::Gold);
		}
	}
}
} // namespace rlnet_detail

void RiverLine::EmbercrestPage(double Now)
{
	const int Today = net::DayOf(World);
	const life::State& L = S.Life;
	const life::LiveEntry* Mine = live::ActiveEntry(L);
	const live::Occurrence Entered = Mine ? live::FindOccurrence(Mine->Id) : live::Occurrence();
	const std::vector<live::Occurrence> Next = LiveAhead(S, World, 48.0);
	// The day shown: the one picked while it's this week, else the entry's day, else the next day with an event the
	// bus can still make.
	if (LiveDay < Today || LiveDay > Today + 6)
	{
		const int Day = Entered.Valid() ? Entered.Day : !Next.empty() ? Next.front().Day : Today;
		ShowLiveDay(std::min(std::max(Day, Today), Today + 6), std::string(), Now);
	}
	const Color Ember = Hex(EcEmberHot);
	const float In = NetEase((Now - PageAt) / 0.8);

	// The banner: the mark over the ranges, what the room is, and where the player stands with it.
	const Rect B{24.0f, 84.0f, 1052.0f, 152.0f};
	C->GlowRoundRect({B.X, B.Y + 8.0f, B.W, B.H}, 16.0f, Rgba(0, 0, 0, 0.45f), 22.0f);
	C->FillRoundRect(B, 16.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, Hex(EcMidnight), Mix(Hex(EcNavy), Hex(EcEmberDeep), 0.2f)));
	C->PushClip(B);
	for (int K = 0; K < 28; ++K)
	{
		// Stars over the ranges, each on its own beat.
		const uint32_t H = live::FaceHash("star" + std::to_string(K));
		const float Sx = B.X + 150.0f + Nf(H % 880u);
		const float Sy = B.Y + 8.0f + Nf((H >> 10) % 90u);
		C->FillCircle(Sx, Sy, 0.8f + Nf((H >> 20) % 3u) * 0.35f, Rgba(235, 231, 223, 0.18f + 0.14f * Nf(std::sin(Now * 0.9 + K))));
	}
	const Vec2 Glow{B.X + 86.0f, B.Y + 64.0f};
	C->FillEllipse(Glow.X, Glow.Y, 170.0f, 120.0f, Paint::Radial(Glow, 0.0f, Glow, 170.0f, Hex(EcEmberHot, 0.3f), 0.45f, Hex(EcEmber, 0.08f), Hex(EcEmber, 0.0f)));
	for (int Range = 0; Range < 2; ++Range)
	{
		std::vector<Vec2> Ridge;
		for (float X = B.X; X <= B.X + B.W + 24.0f; X += 24.0f)
		{
			const float Phase = Nf(Range) * 2.1f;
			Ridge.push_back({X, B.Y + B.H - 40.0f + Nf(Range) * 16.0f + 10.0f * std::sin(X * 0.011f + Phase) + 6.0f * std::sin(X * 0.037f + 1.3f + Phase)});
		}
		Ridge.push_back({B.X + B.W + 24.0f, B.Y + B.H});
		Ridge.push_back({B.X, B.Y + B.H});
		C->FillPolygon(Ridge, Range == 0 ? Hex(EcSlate, 0.55f) : Hex(EcMidnight, 0.85f));
	}
	C->PopClip();
	C->StrokeRoundRect(B, 16.0f, NetA(Ember, 0.35f), 1.0f);
	NetEmbercrest(*C, B.X + 86.0f, B.Y + 74.0f + (1.0f - In) * 8.0f, 112.0f, Hex(EcMidnight));
	const float Tx = B.X + 168.0f;
	NetSpaced(*C, "EMBERCREST CASINO \xC2\xB7 CARD ROOM", Tx, B.Y + 36.0f, 11.0f, 800, Ember, 1.8f);
	UI.Text("The Embercrest", Tx, B.Y + 76.0f, Ts(36.0f, 900, Hex(EcAsh)));
	NetParagraph(*C,
		"Two or three 6-max freezeouts a day, deep stacks, sixty to a hundred and twenty runners. The desk opens three hours before cards and takes entries until late registration closes.",
		Tx, B.Y + 100.0f, 540.0f, 13.5f, 500, Hex(0xc3cedf), 18.0f, 2);
	UI.Text("The 14 bus: 20 minutes and " + NetMoney(live::BusFareCents) + " each way", Tx, B.Y + 140.0f, Ts(12.5f, 700, Hex(EcIce)));
	{
		const Rect Rb{B.X + B.W - 318.0f, B.Y + 18.0f, 298.0f, B.H - 36.0f};
		UI.RRect(Rb, 12.0f, Rgba(4, 8, 16, 0.6f), NetA(Ember, 0.4f));
		std::string Kicker;
		std::string Big;
		std::string Sub;
		if (Mine && Entered.Valid())
		{
			const LiveCard Card = LiveCardOf(S, Entered, World);
			Kicker = "YOU'RE REGISTERED";
			Big = std::string(Entered.T->Short) + " \xC2\xB7 " + net::TimeLabel(Entered.Start);
			Sub = Card.When + " \xC2\xB7 paid " + NetMoney(Mine->PaidCents) + (Mine->ArrivedAt > 0.0 ? " \xC2\xB7 the night's under way" : "");
		}
		else if (L.LiveEvents > 0)
		{
			Kicker = "YOUR NIGHTS HERE";
			Big = std::to_string(L.LiveEvents) + (L.LiveEvents == 1 ? " night" : " nights") + " \xC2\xB7 " + std::to_string(L.LiveCashes) + (L.LiveCashes == 1 ? " cash" : " cashes");
			Sub = (L.LiveBestPlace > 0 ? "Best finish " + Ordinal(L.LiveBestPlace) + " \xC2\xB7 " : std::string()) +
				(L.LiveWonCents > 0 ? "won " + NetMoney(L.LiveWonCents) + " in prizes" : std::string("no cash yet"));
		}
		else if (!Next.empty())
		{
			const live::Occurrence& O = Next.front();
			const LiveCard Card = LiveCardOf(S, O, World);
			Kicker = "NEXT THE BUS CAN MAKE";
			Big = std::string(O.T->Short) + " \xC2\xB7 " + net::TimeLabel(O.Start);
			Sub = Card.When + " \xC2\xB7 " + (O.Start > World ? "cards in " + net::Countdown(O.Start - World) : "late reg till " + net::TimeLabel(O.LateRegEnds));
		}
		NetSpaced(*C, Kicker, Rb.X + 20.0f, Rb.Y + 30.0f, 10.5f, 800, Ember, 1.6f);
		UI.Text(Big, Rb.X + 20.0f, Rb.Y + 62.0f, Ts(20.0f, 800, Hex(EcAsh), Align::Left, Baseline::Alphabetic, false, Rb.W - 40.0f));
		UI.Text(Sub, Rb.X + 20.0f, Rb.Y + 88.0f, Ts(13.0f, 600, Hex(EcIce), Align::Left, Baseline::Alphabetic, false, Rb.W - 40.0f));
	}

	// The day shown, a step at a time through the week (or straight to a day from the week on the right).
	const std::vector<live::Occurrence> Day = live::Occurrences(LiveDay);
	for (int Dir = -1; Dir <= 1; Dir += 2)
	{
		const int To = LiveDay + Dir;
		const bool Can = To >= Today && To <= Today + 6;
		const Rect Sr{Dir < 0 ? 24.0f : 60.0f, 248.0f, 30.0f, 30.0f};
		const Ui::ClickState St = UI.Clickable(Dir < 0 ? "lprev" : "lnext", Sr, Can);
		if (St.Clicked)
		{
			ShowLiveDay(To, std::string(), Now);
		}
		UI.RRect(Sr, 15.0f, St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.03f), Can ? pal::Line : Rgba(255, 255, 255, 0.04f));
		NetChevron(*C, Sr.X + 15.0f, Sr.Y + 15.0f, 11.0f, Dir < 0 ? 1 : 0, Can ? (St.Hover ? pal::Ink : pal::Muted) : pal::Dim);
	}
	const std::string DayName = LiveDay == Today ? "Today" : LiveDay == Today + 1 ? "Tomorrow" : std::string(net::WeekdayName(LiveDay, true));
	const float Dw = UI.Text(DayName, 104.0f, 270.0f, Ts(21.0f, 800, pal::Ink));
	UI.Text(LiveDay <= Today + 1 ? std::string(net::WeekdayName(LiveDay, true)) + ", " + net::DateLabel(LiveDay) : net::DateLabel(LiveDay), 114.0f + Dw, 270.0f, Ts(15.0f, 600, pal::Muted));
	const bool RentDay = (L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice) && net::DayOf(L.RentDeadline - 1.0) == LiveDay;
	UI.Text(std::to_string(Day.size()) + " events" + (RentDay ? "  \xC2\xB7  rent due at midnight" : ""), 1076.0f, 270.0f, Ts(13.0f, 600, RentDay ? pal::Red : pal::Muted, Align::Right));
	const float CardH = 214.0f;
	for (size_t I = 0; I < Day.size() && I < 3; ++I)
	{
		LiveEventCard(Day[I], {24.0f, 282.0f + Nf(I) * (CardH + 12.0f), 1052.0f, CardH}, Now, static_cast<int>(I));
	}
	if (Day.size() < 3)
	{
		const float Top = 282.0f + Nf(Day.size()) * (CardH + 12.0f);
		LiveRoomRules({24.0f, Top, 1052.0f, 950.0f - Top});
	}

	LiveWeek({1092.0f, 84.0f, 484.0f, 866.0f}, Now);
}

void RiverLine::LiveEventCard(const live::Occurrence& O, const Rect& Slot, double Now, int Index)
{
	if (!O.Valid())
	{
		return;
	}
	const LiveCard Card = CardFor(O);
	const live::EventTemplate& T = *O.T;
	const Color Ember = Hex(EcEmberHot);
	const Color Col = LiveColor(T);
	const float Ci = NetEase((Now - std::max(PageAt, LiveDayAt) - 0.07 * static_cast<double>(Index)) / 0.45);
	const Rect R{Slot.X + (1.0f - Ci) * 24.0f, Slot.Y, Slot.W, Slot.H};
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * Ci);
	const bool Picked = O.Id == LivePick;
	const bool Gone = !Card.Registered && Card.Phase >= LiveCard::Closed;
	C->GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 14.0f, Rgba(0, 0, 0, 0.35f), 16.0f);
	if (Picked)
	{
		const float Pulse = 0.5f + 0.5f * Nf(std::sin(Now * 3.0));
		C->GlowRoundRect(R, 14.0f, NetA(Col, 0.12f + 0.1f * Pulse), 14.0f);
	}
	UI.RRect(R, 14.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(Hex(0x101a2b), Col, Card.Registered ? 0.14f : 0.06f), Hex(0x0d1729)),
		Picked || Card.Registered ? NetA(Col, 0.75f) : pal::Line, Picked ? 1.5f : 1.0f);
	C->FillRoundRect({R.X + 1.0f, R.Y + 14.0f, 4.0f, R.H - 28.0f}, 2.0f, Gone ? NetA(Col, 0.4f) : Col);

	// When, where it stands, and the field.
	UI.Text(net::TimeLabel(O.Start), R.X + 26.0f, R.Y + 44.0f, Ts(26.0f, 800, Gone ? pal::Muted : pal::Ink, Align::Left, Baseline::Alphabetic, true));
	UI.Text(Card.When + " \xC2\xB7 " + net::DateLabel(O.Day), R.X + 26.0f, R.Y + 66.0f, Ts(13.0f, 600, pal::Muted));
	std::string Pill;
	Color Pc = pal::Dim;
	if (Card.Registered)
	{
		Pill = "YOU'RE IN";
		Pc = Ember;
	}
	else if (Card.Entry)
	{
		Pill = Card.Entry->State == life::LiveEntry::Refunded ? "REFUNDED" : Card.Entry->PrizeCents > 0 ? "CASHED" : "PLAYED";
		Pc = Card.Entry->PrizeCents > 0 ? pal::Gold : pal::Muted;
	}
	else if (Card.Cancelled)
	{
		Pill = "CALLED OFF";
		Pc = NetA(pal::Red, 0.8f);
	}
	else
	{
		switch (Card.Phase)
		{
		case LiveCard::Soon: Pill = "DESK " + NetUpper(LiveTime(O.DeskOpens)); Pc = Hex(0x6b7a93); break;
		case LiveCard::Desk: Pill = "REGISTERING"; Pc = Hex(0x4f9bff); break;
		case LiveCard::LateReg: Pill = "LATE REG \xC2\xB7 " + NetHms(O.LateRegEnds - live::TravelMinutes - World) + " LEFT"; Pc = pal::Green; break;
		case LiveCard::Closed: Pill = "RUNNING"; Pc = Hex(0x9aa7bd); break;
		default: Pill = "OVER"; Pc = pal::Dim; break;
		}
	}
	NetPill(*C, Pill, R.X + 26.0f, R.Y + 80.0f, Pc, false, 10.0f);
	const std::string FieldBig = Card.Planned ? (Card.Entry || Card.Cancelled ? Grouped(Card.Field) : "~" + Grouped(Card.Field)) : std::to_string(T.FieldLo) + "\xE2\x80\x93" + std::to_string(T.FieldHi);
	UI.Text(FieldBig, R.X + 26.0f, R.Y + 140.0f, Ts(24.0f, 800, Gone ? pal::Muted : pal::Ink, Align::Left, Baseline::Alphabetic, true));
	NetSpaced(*C, Card.Entry && Card.Planned ? "IN THE FIELD" : Card.Cancelled ? "CAME" : !Card.Planned ? "USUAL FIELD" : Card.Phase >= LiveCard::Closed ? "FIELD" : "EXPECTED", R.X + 26.0f,
		R.Y + 158.0f, 9.5f, 800, pal::Dim, 1.2f);

	// The event and its price.
	const float Mx = R.X + 196.0f;
	UI.Text(LiveTitle(T), Mx, R.Y + 42.0f, Ts(22.0f, 900, Gone ? pal::Muted : pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 196.0f - 230.0f));
	NetSpaced(*C, NetUpper(std::string(T.Speed)) + " \xC2\xB7 6-MAX FREEZEOUT \xC2\xB7 BIG-BLIND ANTE FROM LEVEL 3", Mx, R.Y + 64.0f, 10.0f, 800, Col, 1.3f);
	UI.Text(NetMoney(T.BuyInCents), R.X + R.W - 26.0f, R.Y + 46.0f, Ts(30.0f, 900, Gone ? pal::Muted : pal::Gold, Align::Right, Baseline::Alphabetic, true));
	UI.Text(Card.Split, R.X + R.W - 26.0f, R.Y + 68.0f, Ts(13.0f, 600, pal::Muted, Align::Right));

	// The middle flips (next_segment_spec §8.3, P1): the room's facts, its structure level by level, or the payouts for
	// the field expected. Three small tabs on the price's second line, left of the split.
	int& Flip = NetChoicesOf(this).Flip[O.Id];
	{
		const char* Faces[3] = {"FACTS", "STRUCTURE", "PAYOUTS"};
		float Fx = R.X + R.W - 26.0f - UI.Measure(Card.Split, 13.0f, 600) - 18.0f;
		for (int K = 2; K >= 0; --K)
		{
			const float Fw = C->Measure(Faces[K], 9.0f, 800) + 1.0f * Nf(std::string(Faces[K]).size()) + 16.0f;
			Fx -= Fw;
			const Rect Fr{Fx, R.Y + 55.0f, Fw, 18.0f};
			const Ui::ClickState Fc = UI.Clickable("lflip" + std::to_string(K) + O.Id, Fr);
			if (Fc.Clicked)
			{
				Flip = K;
			}
			const bool On = Flip == K;
			UI.RRect(Fr, 9.0f, On ? NetA(Col, 0.22f) : Fc.Hover ? Rgba(255, 255, 255, 0.07f) : Rgba(255, 255, 255, 0.02f), On ? NetA(Col, 0.7f) : pal::Line);
			NetSpaced(*C, Faces[K], Fr.X + Fw / 2.0f, Fr.Y + 12.5f, 9.0f, 800, On ? pal::Ink : pal::Muted, 1.0f, Align::Center);
			Fx -= 6.0f;
		}
	}
	const Color FactInk = Gone ? pal::Muted : pal::Ink;
	if (Flip == 1)
	{
		// The structure: the first eight levels (late registration through the levels in ember, the break marked).
		const std::vector<Level>& Levels = live::DeepstackLevels();
		const float Lw8 = (R.X + R.W - 26.0f - Mx) / 8.0f;
		for (int K = 0; K < 8 && K < static_cast<int>(Levels.size()); ++K)
		{
			const Level& Lv = Levels[static_cast<size_t>(K)];
			const float Lx = Mx + Nf(K) * Lw8;
			const bool Late = K < T.LateRegLevels;
			NetSpaced(*C, "LEVEL " + std::to_string(K + 1), Lx, R.Y + 98.0f, 9.0f, 800, Late ? NetA(Ember, 0.85f) : pal::Dim, 1.1f);
			UI.Text(Grouped(Lv.Sb) + "/" + Grouped(Lv.Bb), Lx, R.Y + 115.0f, Ts(14.0f, 700, FactInk, Align::Left, Baseline::Alphabetic, true, Lw8 - 8.0f));
			UI.Text(Lv.Ante > 0 ? "ante " + Grouped(Lv.Ante) : std::string("no ante"), Lx, R.Y + 130.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Lw8 - 8.0f));
			if (T.BreakEvery > 0 && (K + 1) % T.BreakEvery == 0 && K < 7)
			{
				C->FillRect({Lx + Lw8 - 6.0f, R.Y + 88.0f, 1.5f, 46.0f}, NetA(Col, 0.6f));
			}
		}
	}
	else if (Flip == 2)
	{
		// The payouts for the field as it stands (the entry's, the world's plan, the room's usual crowd), as the room pays.
		// (The player counts in it only while they could still register: a closed or finished night pays its own field.)
		const bool Joins = !Card.Entry && Card.Phase <= LiveCard::LateReg;
		const int Field = std::max(T.MinEntrants, Card.Planned ? Card.Field + (Joins ? 1 : 0) : (T.FieldLo + T.FieldHi) / 2 + (Joins ? 1 : 0));
		const Chips Seat = T.BuyInCents - T.FeeCents;
		const std::vector<Chips> Pay = PayoutTable(Seat * Field, Field, static_cast<Chips>(std::llround(static_cast<double>(Seat) * 1.4)));
		const float Pw8 = (R.X + R.W - 26.0f - Mx) / 8.0f;
		for (int K = 0; K < 8; ++K)
		{
			const float Px = Mx + Nf(K) * Pw8;
			const std::string Head = K < 6 ? NetUpper(Ordinal(K + 1)) : K == 6 ? std::string("MIN CASH") : std::string("PAID");
			const std::string Value = K < 6 ? (static_cast<size_t>(K) < Pay.size() ? NetMoney(Pay[static_cast<size_t>(K)]) : std::string("\xE2\x80\x94"))
				: K == 6 ? (Pay.empty() ? std::string("\xE2\x80\x94") : NetMoney(Pay.back())) : std::to_string(Pay.size());
			NetSpaced(*C, Head, Px, R.Y + 98.0f, 9.0f, 800, pal::Dim, 1.1f);
			UI.Text(Value, Px, R.Y + 115.0f, Ts(14.0f, 800, K == 0 && !Gone ? pal::Gold : FactInk, Align::Left, Baseline::Alphabetic, true, Pw8 - 8.0f));
		}
		UI.Text("With " + Grouped(Field) + " runners: a " + NetMoney(Seat * Field) + " prize pool, the buy-in less the fee.", Mx, R.Y + 131.0f,
			Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.X + R.W - 26.0f - 230.0f - 20.0f - Mx));
	}
	else
	{
		// The room's facts.
		const float Cw = (R.X + R.W - 26.0f - Mx) / 3.0f;
		const std::pair<const char*, std::string> Facts[6] = {
			{"STACK", Grouped(T.StartingStack) + " \xC2\xB7 " + std::to_string(Card.StackBb) + " BB"},
			{"LEVELS", std::to_string(static_cast<int>(T.LevelMinutes)) + " minutes"},
			{"BREAKS", LiveBreaks(T)},
			{"LATE REG", "until " + net::TimeLabel(O.LateRegEnds) + " (" + std::to_string(T.LateRegLevels) + " levels)"},
			{"MINIMUM", std::to_string(T.MinEntrants) + " \xC2\xB7 else refunded"},
			{"DEEP RUN", "about " + LiveHours(Card.RunMinutes / 60.0) + ", to " + net::TimeLabel(Card.Ends)},
		};
		for (int K = 0; K < 6; ++K)
		{
			const float Fx = Mx + Nf(K % 3) * Cw;
			const float Fy = R.Y + 102.0f + Nf(K / 3) * 24.0f;
			NetSpaced(*C, Facts[K].first, Fx, Fy, 9.5f, 800, pal::Dim, 1.2f);
			UI.Text(Facts[K].second, Fx + 72.0f, Fy, Ts(14.0f, 700, FactInk, Align::Left, Baseline::Alphabetic, false, Cw - 80.0f));
		}
	}

	// Getting there, and who's going.
	const float Lw = R.X + R.W - 26.0f - 230.0f - 20.0f - Mx;
	std::string Bus = "The 14 bus: " + NetMoney(live::BusFareCents) + " and 20 minutes each way";
	if (!Card.Entry && Card.Phase <= LiveCard::LateReg)
	{
		Bus += Card.Phase == LiveCard::LateReg ? "  \xC2\xB7  leave by " + net::TimeLabel(O.LateRegEnds - live::TravelMinutes) + " for late reg"
											   : "  \xC2\xB7  leave by " + net::TimeLabel(O.Start - live::TravelMinutes) + " for cards";
	}
	UI.Text(Bus, Mx, R.Y + 156.0f, Ts(13.0f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, Lw));
	std::string People;
	if (!Card.Known.empty())
	{
		// The regulars down for it, each with the one glyph the room would show by them (§5.6: Room scope).
		const Color Ink = Hex(0xe9c79a);
		const TextStyle Style = Ts(13.0f, 600, Ink);
		const float End = Mx + Lw;
		float Px = Mx + UI.Text("You know ", Mx, R.Y + 178.0f, Style);
		const world::World& W = S.Living();
		for (size_t K = 0; K < Card.Known.size() && K < 3; ++K)
		{
			const std::string Lead = K == 0 ? std::string() : ", ";
			const std::string Name = Lead + Card.Known[K].first;
			const std::string Label = " (" + Card.Known[K].second + ")";
			const int Who = W.Find(Card.Known[K].first);
			const badge::Badge* B = Who >= 0 ? badge::Top(NetBadgesOf(S, Who), badge::Room, badge::FrameNone) : nullptr;
			const float Need = UI.Measure(Name, 13.0f, 600) + (B ? 18.0f : 0.0f) + UI.Measure(Label, 13.0f, 600);
			if (Px + Need > End)
			{
				UI.Text("+" + std::to_string(Card.Known.size() - K), Px + 6.0f, R.Y + 178.0f, Style);
				break;
			}
			Px += UI.Text(Name, Px, R.Y + 178.0f, Style);
			if (B)
			{
				badgeart::Draw(*C, *B, Px + 9.0f, R.Y + 173.0f, 14.0f, Now);
				Px += 18.0f;
			}
			Px += UI.Text(Label, Px, R.Y + 178.0f, Ts(13.0f, 600, NetA(Ink, 0.75f)));
			if (K == 2 && Card.Known.size() > 3)
			{
				UI.Text(" +" + std::to_string(Card.Known.size() - 3), Px, R.Y + 178.0f, Style);
			}
		}
	}
	else if (Card.Cancelled)
	{
		People = "Nobody played it: every entry was refunded.";
	}
	else if (!Card.Planned && O.Day > net::DayOf(World) + 1)
	{
		People = "Who's going firms up the day before.";
	}
	else if (Card.Phase < LiveCard::Over)
	{
		People = "Nobody you know is down for it.";
	}
	if (!People.empty())
	{
		UI.Text(People, Mx, R.Y + 178.0f, Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Lw));
	}

	// The button: registering (Session::GoToLive, the Burner's own path) behind a confirm, or the way back to an entry. A
	// reason too long for the button's second line goes to the strip along the bottom.
	const Rect Bb{R.X + R.W - 26.0f - 230.0f, R.Y + 138.0f, 230.0f, 42.0f};
	const Chips Fare = live::BusFareCents;
	const float SubSize = 17.0f * 0.72f;
	const bool WhyFits = Card.Why.empty() || UI.Measure(Card.Why, SubSize, 600, true) <= Bb.W - 16.0f;
	std::vector<LiveWarning> Strip = Card.Warnings;
	if (!WhyFits && !Card.Entry && Card.Phase <= LiveCard::LateReg && S.BankrollCents >= Card.Cost)
	{
		Strip.insert(Strip.begin(), LiveWarning{Card.Why, 2});
	}
	// Who pays (next_segment_spec §2.12, §3.9): after anything that stops the night, before the notes. A cover says how
	// many are left this period.
	const bool Playable = Card.Registered || (!Card.Entry && !Card.Cancelled && Card.Phase <= LiveCard::LateReg);
	const std::string Paying = Playable ? LivePaying(Card) : std::string();
	if (!Paying.empty())
	{
		auto At = Strip.begin();
		while (At != Strip.end() && At->Severity >= 1)
		{
			++At;
		}
		Strip.insert(At, LiveWarning{Paying, 0});
	}
	if (Card.Entry && !Card.Registered)
	{
		UI.Text(Card.Status, Bb.X + Bb.W, Bb.Y + 28.0f, Ts(17.0f, 800, Card.Entry->PrizeCents > 0 ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, false, Bb.W + 60.0f));
	}
	else if (Card.Registered)
	{
		ButtonOpts B;
		B.Kind = ButtonKind::Primary;
		B.Size = 17.0f;
		B.Enabled = Card.Why.empty();
		B.Sub = !B.Enabled ? (WhyFits ? Card.Why : std::string()) : "Paid " + NetMoney(Card.Entry->PaidCents);
		if (UI.Button("lgo" + O.Id, Bb, "Go back", B))
		{
			GoToLive(O.Id, Now);
		}
	}
	else if (Card.Phase == LiveCard::Over)
	{
		UI.Text(Card.Status, Bb.X + Bb.W, Bb.Y + 28.0f, Ts(15.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, false, Bb.W + 60.0f));
	}
	else if (Card.Phase == LiveCard::Closed)
	{
		ButtonOpts B;
		B.Kind = ButtonKind::Secondary;
		B.Size = 17.0f;
		B.Enabled = false;
		B.Sub = "cards till about " + net::TimeLabel(Card.Ends);
		UI.Button("lshut" + O.Id, Bb, "Registration closed", B);
	}
	else if (LiveConfirm == O.Id)
	{
		// Exactly what leaves the bank now leads the strip, the warnings after it.
		Strip.insert(Strip.begin(), LiveWarning{LiveLeavesNow(Card, S.BankrollCents), -1});
		const float Hw = (Bb.W - 10.0f) / 2.0f;
		ButtonOpts Ok;
		Ok.Kind = ButtonKind::Gold;
		Ok.Size = 17.0f;
		Ok.Enabled = Card.Why.empty();
		if (UI.Button("lok" + O.Id, {Bb.X, Bb.Y, Hw, Bb.H}, "Confirm", Ok))
		{
			GoToLive(O.Id, Now);
		}
		ButtonOpts No;
		No.Kind = ButtonKind::Ghost;
		No.Size = 17.0f;
		if (UI.Button("lno" + O.Id, {Bb.X + Hw + 10.0f, Bb.Y, Hw, Bb.H}, "Cancel", No))
		{
			LiveConfirm.clear();
		}
	}
	else
	{
		ButtonOpts B;
		B.Enabled = Card.Why.empty();
		B.Kind = B.Enabled ? ButtonKind::Primary : ButtonKind::Secondary;
		B.Size = 17.0f;
		const LiveRegisterLabel Label = LiveRegisterLabelOf(Card, Fare);
		B.Sub = B.Enabled ? Label.Sub : WhyFits ? Card.Why : std::string();
		if (UI.Button("lreg" + O.Id, Bb, Label.Text, B))
		{
			LiveConfirm = O.Id;
		}
	}

	// What it costs against the money (confirming, what leaves the bank now leads it, in gold). At its right end, the way
	// to sell pieces of it (the Backing page, opened on this event) while it's the player's own money to sell.
	const Rect Wr{R.X + 18.0f, R.Y + R.H - 28.0f, R.W - 36.0f, 20.0f};
	float StripEnd = Wr.X + Wr.W;
	if ((Card.Sellable || Card.Asked > 0) && !Card.Entry)
	{
		const std::string Sell = Card.Filled > 0 ? NetPermille(Card.Filled) + " sold  \xE2\x80\xBA" : Card.Asked > 0 ? "Selling " + NetPermille(Card.Asked) + "  \xE2\x80\xBA" : std::string("Sell action  \xE2\x80\xBA");
		const float Sw = UI.Measure(Sell, 12.5f, 800) + 24.0f;
		const Rect Sr{Wr.X + Wr.W - Sw, Wr.Y - 2.0f, Sw, Wr.H + 4.0f};
		const Ui::ClickState Sc = UI.Clickable("lsell" + O.Id, Sr);
		UI.RRect(Sr, 8.0f, Sc.Hover ? NetA(pal::Accent, 0.2f) : NetA(pal::Accent, 0.08f), NetA(pal::Accent, Sc.Hover ? 0.8f : 0.45f));
		UI.Text(Sell, Sr.X + Sw / 2.0f, Sr.Y + Sr.H / 2.0f + 1.0f, Ts(12.5f, 800, Sc.Hover ? pal::Ink : pal::Accent, Align::Center, Baseline::Middle));
		if (Sc.Clicked)
		{
			backing_detail::Preselect(UI, O.Id);
			OpenPage(Page::Backing, Now);
		}
		StripEnd = Sr.X - 8.0f;
	}
	if (!Strip.empty())
	{
		const int Lead = Strip.front().Severity;
		UI.RRect({Wr.X, Wr.Y, StripEnd - Wr.X, Wr.H}, 6.0f, Lead < 0 ? Rgba(242, 193, 78, 0.08f) : Lead >= 2 ? Rgba(239, 77, 90, 0.1f) : Lead == 1 ? Rgba(242, 138, 58, 0.09f) : Rgba(255, 255, 255, 0.03f));
		float X = Wr.X + 10.0f;
		for (const LiveWarning& Wn : Strip)
		{
			const Color Wc = Wn.Severity < 0 ? pal::Gold : Wn.Severity >= 2 ? pal::Red : Wn.Severity == 1 ? pal::Orange : Hex(0x9fb0c8);
			if (Wn.Severity >= 1)
			{
				C->FillCircle(X + 6.0f, Wr.Y + 10.0f, 6.0f, Wc);
				UI.Text("!", X + 6.0f, Wr.Y + 10.5f, Ts(10.0f, 900, Hex(0x0d1729), Align::Center, Baseline::Middle));
				X += 18.0f;
			}
			else if (Wn.Severity == 0)
			{
				NetCheck(*C, X + 6.0f, Wr.Y + 10.0f, 10.0f, Wc);
				X += 18.0f;
			}
			X += UI.Text(Wn.Text, X, Wr.Y + 14.5f, Ts(12.5f, 700, Wc, Align::Left, Baseline::Alphabetic, false, StripEnd - 10.0f - X)) + 22.0f;
			if (X > StripEnd - 80.0f)
			{
				break;
			}
		}
	}
	C->SetAlpha(A0);
}

void RiverLine::LiveRoomRules(const Rect& R)
{
	if (R.H < 120.0f)
	{
		return;
	}
	Panel(R, 14.0f);
	Section("HOW THE ROOM RUNS", R.X + 24.0f, R.Y + 34.0f, Hex(EcEmberHot));
	const std::string Rules[4] = {
		"Every event is a 6-max freezeout: one entry, no rebuys. A big-blind ante starts at level 3, and there's a break every six levels.",
		"The desk opens three hours before cards and takes entries until late registration closes. You can hold one entry at a time.",
		"An event that doesn't reach its stated minimum is cancelled, and every entry is refunded in full.",
		"The 14 bus is 20 minutes and " + NetMoney(live::BusFareCents) + " each way. The fare home is paid when you leave the room.",
	};
	float Y = R.Y + 64.0f;
	for (const std::string& Rule : Rules)
	{
		if (Y > R.Y + R.H - 14.0f)
		{
			break;
		}
		C->FillCircle(R.X + 30.0f, Y - 5.0f, 3.0f, Hex(EcEmberHot));
		Y = NetParagraph(*C, Rule, R.X + 44.0f, Y, R.W - 72.0f, 14.0f, 500, Hex(0xc3cedf), 19.0f, 2) + 9.0f;
	}
}

void RiverLine::LiveWeek(const Rect& R, double Now)
{
	const int Today = net::DayOf(World);
	const life::State& L = S.Life;
	const Color Ember = Hex(EcEmberHot);
	const double Since = Now - PageAt;
	Panel(R);
	// Two pages (§5.3, §5.6): the week, and the room's own: the player's club card and this month's leaders.
	NetChoices& Ch = NetChoicesOf(this);
	{
		const char* Tabs[2] = {"THE WEEK", "LEADERS"};
		float Tx = R.X + 24.0f;
		for (int K = 0; K < 2; ++K)
		{
			const float Tw = C->Measure(Tabs[K], 12.0f, 800) + 1.6f * Nf(std::string(Tabs[K]).size()) + 11.0f;
			const Ui::ClickState Tc = UI.Clickable("lweektab" + std::to_string(K), {Tx - 4.0f, R.Y + 16.0f, Tw + 8.0f, 30.0f});
			if (Tc.Clicked && Ch.WeekTab != K)
			{
				Ch.WeekTab = K;
				Ch.WeekTabAt = Now;
			}
			const bool On = Ch.WeekTab == K;
			C->FillRoundRect({Tx, R.Y + 25.0f, 3.0f, 13.0f}, 1.5f, On ? Ember : pal::Line);
			NetSpaced(*C, Tabs[K], Tx + 11.0f, R.Y + 36.0f, 12.0f, 800, On ? pal::Ink : Tc.Hover ? Hex(0xc3cedf) : pal::Muted, 1.6f);
			Tx += Tw + 22.0f;
		}
	}
	if (Ch.WeekTab == 1)
	{
		LiveLeaders(*C, UI, this, S, R, World, Now, Now - std::max(Ch.WeekTabAt, PageAt));
		return;
	}
	int Count = 0;
	for (int D = Today; D <= Today + 6; ++D)
	{
		Count += static_cast<int>(live::Occurrences(D).size());
	}
	UI.Text(std::to_string(Count) + " events", R.X + R.W - 24.0f, R.Y + 36.0f, Ts(13.0f, 600, pal::Muted, Align::Right));
	const bool RentLive = L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice;
	const int RentDay = net::DayOf(L.RentDeadline - 1.0);
	float Y = R.Y + 54.0f;
	for (int D = Today; D <= Today + 6; ++D)
	{
		const std::vector<live::Occurrence> Os = live::Occurrences(D);
		const float H = 28.0f + 25.0f * Nf(Os.size());
		const bool On = D == LiveDay;
		const Rect Dr{R.X + 12.0f, Y, R.W - 24.0f, H};
		const float Di = NetEase((Since - 0.04 * static_cast<double>(D - Today)) / 0.4);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Di);
		if (On)
		{
			UI.RRect(Dr, 10.0f, NetA(Ember, 0.08f), NetA(Ember, 0.35f));
		}
		const Ui::ClickState Ds = UI.Clickable("lday" + std::to_string(D), {Dr.X, Dr.Y, Dr.W, 26.0f});
		if (Ds.Clicked)
		{
			ShowLiveDay(D, std::string(), Now);
		}
		const std::string Name = D == Today ? "TODAY" : D == Today + 1 ? "TOMORROW" : NetUpper(net::WeekdayName(D, true));
		const float Nw = NetSpaced(*C, Name, Dr.X + 12.0f, Y + 19.0f, 10.5f, 800, On || Ds.Hover ? Ember : pal::Muted, 1.4f);
		UI.Text(net::DateLabel(D), Dr.X + 22.0f + Nw, Y + 19.0f, Ts(11.5f, 600, pal::Dim));
		if (RentLive && D == RentDay)
		{
			const std::string Due = (L.RentStage == life::Rent::FinalNotice ? "LOCKS CHANGE \xC2\xB7 " : "RENT DUE \xC2\xB7 ") + NetMoney(L.RentDueCents);
			const float Pw = C->Measure(Due, 9.0f, 700) + 9.0f * 1.3f;
			NetPill(*C, Due, Dr.X + Dr.W - 10.0f - Pw, Y + 6.0f, pal::Red, false, 9.0f);
		}
		for (size_t K = 0; K < Os.size(); ++K)
		{
			const live::Occurrence& O = Os[K];
			const Rect Row{Dr.X + 4.0f, Y + 26.0f + Nf(K) * 25.0f, Dr.W - 8.0f, 24.0f};
			const Ui::ClickState St = UI.Clickable("lev" + O.Id, Row);
			if (St.Clicked)
			{
				ShowLiveDay(D, O.Id, Now);
			}
			if (St.Hover || O.Id == LivePick)
			{
				UI.RRect(Row, 6.0f, Rgba(255, 255, 255, O.Id == LivePick ? 0.06f : 0.04f));
			}
			const life::LiveEntry* E = live::EntryFor(L, O.Id);
			const bool Off = !E && S.Living().CancellationOf(O.Id) != nullptr;
			const bool Past = Off || O.LateRegEnds < World + live::TravelMinutes;
			C->FillCircle(Row.X + 14.0f, Row.Y + 12.0f, 4.0f, Past && !E ? NetA(LiveColor(*O.T), 0.4f) : LiveColor(*O.T));
			UI.Text(net::TimeLabel(O.Start), Row.X + 28.0f, Row.Y + 17.0f, Ts(12.5f, 700, Past ? pal::Dim : pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(O.T->Short, Row.X + 104.0f, Row.Y + 17.0f, Ts(14.0f, 700, Past && !E ? pal::Muted : pal::Ink, Align::Left, Baseline::Alphabetic, false, 190.0f));
			UI.Text(NetMoney(O.T->BuyInCents), Row.X + Row.W - 92.0f, Row.Y + 17.0f, Ts(13.0f, 800, Past ? pal::Dim : pal::Gold, Align::Right, Baseline::Alphabetic, true));
			// The player's entry, or how far along the night is.
			if (E)
			{
				const std::string Mark = E->State == life::LiveEntry::Registered ? std::string("YOU'RE IN") : E->State == life::LiveEntry::Refunded ? std::string("REFUNDED") : Ordinal(E->Place);
				const Color Mc = E->State == life::LiveEntry::Registered ? Ember : E->PrizeCents > 0 ? pal::Gold : pal::Muted;
				const float Mw = C->Measure(Mark, 9.0f, 700) + 9.0f * 1.3f;
				NetPill(*C, Mark, Row.X + Row.W - 8.0f - Mw, Row.Y + 4.5f, Mc, E->State == life::LiveEntry::Registered, 9.0f);
			}
			else if (Off)
			{
				NetSpaced(*C, "CALLED OFF", Row.X + Row.W - 8.0f, Row.Y + 16.0f, 9.0f, 800, NetA(pal::Red, 0.8f), 1.0f, Align::Right);
			}
			else if (!Past && O.Start <= World)
			{
				NetSpaced(*C, "LATE REG", Row.X + Row.W - 8.0f, Row.Y + 16.0f, 9.0f, 800, pal::Green, 1.0f, Align::Right);
			}
			else if (Past)
			{
				NetCheck(*C, Row.X + Row.W - 18.0f, Row.Y + 12.0f, 9.0f, pal::Dim);
			}
		}
		C->SetAlpha(A0);
		Y += H + 6.0f;
	}

	// The money: the balance against the rent, as the Career page has it.
	const Rect Mb{R.X + 12.0f, R.Y + R.H - 128.0f, R.W - 24.0f, 116.0f};
	if (Mb.Y < Y)
	{
		return;
	}
	UI.RRect(Mb, 12.0f, Rgba(4, 8, 16, 0.45f), pal::Line);
	NetSpaced(*C, "BALANCE", Mb.X + 18.0f, Mb.Y + 28.0f, 10.0f, 800, pal::Muted, 1.4f);
	UI.Text(Money(S.BankrollCents), Mb.X + 18.0f, Mb.Y + 62.0f, Ts(26.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true));
	const bool Evicted = L.RentStage == life::Rent::Evicted;
	const std::string Head = Evicted ? "A KEY BACK \xC2\xB7 " + NetMoney(L.RentDueCents)
		: L.RentStage == life::Rent::FinalNotice ? "FINAL NOTICE \xC2\xB7 " + NetMoney(L.RentDueCents) + " " + NetUpper(net::WeekdayName(RentDay, true))
		: L.RentStage == life::Rent::Paid ? "RENT PAID \xC2\xB7 NEXT DUE " + NetUpper(net::DateLabel(RentDay))
										 : "RENT \xC2\xB7 " + NetMoney(L.RentDueCents) + " DUE " + NetUpper(net::WeekdayName(RentDay, true));
	NetSpaced(*C, Head, Mb.X + Mb.W - 18.0f, Mb.Y + 28.0f, 10.0f, 800, L.RentStage == life::Rent::Paid ? pal::Green : pal::Red, 1.3f, Align::Right);
	const Chips Rent = std::max<Chips>(1, L.RentDueCents);
	const float Frac = Nf(Clamp01(static_cast<double>(S.BankrollCents) / static_cast<double>(Rent)));
	const Rect Bar{Mb.X + 18.0f, Mb.Y + 78.0f, Mb.W - 36.0f, 8.0f};
	UI.RRect(Bar, 4.0f, Rgba(255, 255, 255, 0.08f));
	UI.RRect({Bar.X, Bar.Y, std::max(8.0f, Bar.W * Frac * NetEase(Since / 0.9)), Bar.H}, 4.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, pal::Red, pal::Gold));
	const std::string Note = L.RentStage == life::Rent::Paid ? std::string("Paid up: nothing on the calendar but poker.")
		: S.BankrollCents >= L.RentDueCents ? (Evicted ? std::string("Enough to move back in: the Bank app.") : std::string("Covered: pay it in the Bank app."))
											: NetMoney(L.RentDueCents - S.BankrollCents) + (Evicted ? " to go to move back in" : " to go before the landlord comes");
	UI.Text(Note, Mb.X + 18.0f, Mb.Y + 104.0f, Ts(12.5f, 600, S.BankrollCents >= L.RentDueCents || L.RentStage == life::Rent::Paid ? pal::Green : Hex(0xc3cedf), Align::Left, Baseline::Alphabetic,
		false, Mb.W - 36.0f));
}

// ------------------------------------------------------------------ series

void RiverLine::SeriesPage(double Now)
{
	const net::Network& Net = net::Shared();
	const net::SeriesInfo* Sr = SeriesId.empty() ? Net.CurrentSeries(World) : Net.FindSeries(SeriesId);
	if (!Sr)
	{
		return;
	}
	const int Today = net::DayOf(World);
	const bool Ahead = Today < Sr->FirstDay;
	const bool Over = Today > Sr->LastDay;
	const Color Col = Hex(Sr->Color);
	const Color Col2 = Hex(Sr->Color2);
	const double Since = Now - std::max(PageAt, SeriesAt);
	const float In = NetEase(Since / 0.8);
	if (SeriesDay < Sr->FirstDay || SeriesDay > Sr->LastDay)
	{
		SeriesDay = std::min(std::max(Today, Sr->FirstDay), Sr->LastDay);
	}

	// Series picker: the last one played, the one running, the next ones (and the one shown, wherever it is).
	Section("SERIES", 24.0f, 108.0f, Col);
	std::vector<const net::SeriesInfo*> Near;
	{
		std::vector<const net::SeriesInfo*> All;
		for (const net::SeriesInfo& O : Net.Series())
		{
			All.push_back(&O);
		}
		std::sort(All.begin(), All.end(), [](const net::SeriesInfo* A, const net::SeriesInfo* B) { return A->FirstDay < B->FirstDay; });
		size_t Now0 = All.size();
		for (size_t K = 0; K < All.size(); ++K)
		{
			if (All[K]->LastDay >= Today)
			{
				Now0 = K;
				break;
			}
		}
		const size_t From = Now0 > 0 ? Now0 - 1 : 0;
		for (size_t K = From; K < All.size() && Near.size() < 4; ++K)
		{
			Near.push_back(All[K]);
		}
		if (std::find(Near.begin(), Near.end(), Sr) == Near.end())
		{
			Near.back() = Sr;
			std::sort(Near.begin(), Near.end(), [](const net::SeriesInfo* A, const net::SeriesInfo* B) { return A->FirstDay < B->FirstDay; });
		}
	}
	float Px = NetW - 24.0f;
	for (size_t K = Near.size(); K-- > 0;)
	{
		const net::SeriesInfo& O = *Near[K];
		const std::string Label = O.Name + (net::DayOf(World) > O.LastDay ? "" : net::DayOf(World) >= O.FirstDay ? "  \xC2\xB7  LIVE" : "  \xC2\xB7  " + NetUpper(net::DateLabel(O.FirstDay)));
		const float W = UI.Measure(Label, 13.0f, 700) + 30.0f + 22.0f;
		Px -= W;
		const Rect R{Px, 88.0f, W, 30.0f};
		const Ui::ClickState St = UI.Clickable("series" + O.Id, R);
		if (St.Clicked)
		{
			ShowSeries(O.Id, Now);
		}
		const bool On = &O == Sr;
		UI.RRect(R, 15.0f, On ? NetA(Hex(O.Color), 0.2f) : St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? Hex(O.Color) : pal::Line);
		eventart::SeriesCrest(*C, O, R.X + 18.0f, R.Y + 15.0f, 26.0f, Now);
		UI.Text(Label, R.X + 11.0f + W / 2.0f, R.Y + 15.5f, Ts(13.0f, 700, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle));
		Px -= 8.0f;
	}

	// Banner.
	const Rect B{24.0f, 130.0f, 1552.0f, 222.0f};
	C->GlowRoundRect({B.X, B.Y + 10.0f, B.W, B.H}, 18.0f, Rgba(0, 0, 0, 0.5f), 26.0f);
	C->FillRoundRect(B, 18.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, Mix(Hex(0x07101c), Col, 0.2f), Mix(Hex(0x0a1322), Col2, 0.24f)));
	C->PushClip(B);
	C->FillEllipse(B.X + B.W - 520.0f, B.Y + B.H * 0.5f, 520.0f, 300.0f, Paint::Radial({B.X + B.W - 520.0f, B.Y + B.H * 0.5f}, 0.0f, {B.X + B.W - 520.0f, B.Y + B.H * 0.5f}, 520.0f, NetA(Col2, 0.35f), 0.5f, NetA(Col, 0.08f), NetA(Col, 0.0f)));
	for (int K = 0; K < 6; ++K)
	{
		std::vector<Vec2> Wave;
		for (float X = B.X; X <= B.X + B.W; X += 14.0f)
		{
			Wave.push_back({X, B.Y + 40.0f + Nf(K) * 34.0f + Nf(std::sin(X / 170.0f + Now * 0.5 + K * 0.9)) * 16.0f});
		}
		C->StrokePolyline(Wave, false, NetA(K % 2 ? Col2 : Col, 0.09f), 2.0f);
	}
	UI.Text(Sr->Short, B.X + B.W - 400.0f, B.Y + B.H + 40.0f, Ts(260.0f, 900, NetA(Hex(0xffffff), 0.04f), Align::Right));
	C->PopClip();
	C->StrokeRoundRect(B, 18.0f, NetA(Col, 0.35f), 1.0f);
	// The series' crest.
	C->FillEllipse(B.X + 82.0f, B.Y + 96.0f, 96.0f, 96.0f, Paint::Radial({B.X + 82.0f, B.Y + 96.0f}, 0.0f, {B.X + 82.0f, B.Y + 96.0f}, 96.0f, Rgba(4, 8, 16, 0.55f), 0.55f, Rgba(4, 8, 16, 0.25f), Rgba(4, 8, 16, 0.0f)));
	eventart::SeriesCrest(*C, *Sr, B.X + 82.0f, B.Y + 96.0f + (1.0f - In) * 12.0f, 150.0f, Now);
	UI.Text(Sr->Name, B.X + 150.0f, B.Y + 78.0f, Ts(46.0f, 900, pal::Ink));
	UI.Text(Sr->Tagline, B.X + 150.0f, B.Y + 110.0f, Ts(17.0f, 500, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, 760.0f));
	const int DayCount = Sr->LastDay - Sr->FirstDay + 1;
	std::string State = Ahead ? "Starts in " + net::Countdown(static_cast<double>(Sr->FirstDay) * net::MinutesPerDay - World) : Over ? "Complete" : "Day " + std::to_string(Today - Sr->FirstDay + 1) + " of " + std::to_string(DayCount);
	const std::pair<std::string, std::string> Stats[4] = {
		{NetMoney(static_cast<Chips>(static_cast<double>(Sr->GtdCents) * static_cast<double>(In) / 100.0) * 100), "GUARANTEED"},
		{std::to_string(static_cast<int>(std::round(static_cast<double>(Sr->Events) * static_cast<double>(In)))),
			Sr->Bracelets > 0 ? "EVENTS \xC2\xB7 " + std::to_string(Sr->Bracelets) + " BRACELETS" : Sr->Rings > 0 ? "EVENTS \xC2\xB7 " + std::to_string(Sr->Rings) + " RINGS" : std::string("EVENTS")},
		{net::DateLabel(Sr->FirstDay) + " \xE2\x80\x93 " + net::DateLabel(Sr->LastDay), "DATES"},
		{State, Ahead ? "COUNTDOWN" : "PROGRESS"},
	};
	float Sx = B.X + 150.0f;
	for (int I = 0; I < 4; ++I)
	{
		const float W = UI.Text(Stats[I].first, Sx, B.Y + 166.0f, Ts(I == 0 ? 30.0f : 24.0f, 900, I == 0 ? pal::Gold : pal::Ink));
		const float Lw = NetSpaced(*C, Stats[I].second, Sx, B.Y + 188.0f, 10.5f, 800, pal::Muted, 1.6f);
		Sx += std::max({W, Lw, 120.0f}) + 48.0f;
	}
	// Progress through the series.
	const float Prog = Nf(Clamp01((World - static_cast<double>(Sr->FirstDay) * net::MinutesPerDay) / (static_cast<double>(DayCount) * net::MinutesPerDay)));
	const Rect Bar{B.X + 150.0f, B.Y + B.H - 16.0f, B.W - 150.0f - 440.0f, 5.0f};
	UI.RRect(Bar, 2.5f, Rgba(255, 255, 255, 0.08f));
	if (Prog > 0.0f)
	{
		UI.RRect({Bar.X, Bar.Y, Bar.W * Prog * In, Bar.H}, 2.5f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, Col, Col2));
		C->FillCircle(Bar.X + Bar.W * Prog * In, Bar.Y + 2.5f, 5.0f, Hex(0xffffff));
	}
	// The main event.
	net::EventInstance Main;
	const bool HasMain = Net.Next(Sr->MainEvent, static_cast<double>(Sr->FirstDay) * net::MinutesPerDay, Main);
	if (HasMain)
	{
		const net::EventTemplate& T = Net.TemplateOf(Main);
		const net::LiveState L = Net.Live(Main, World);
		const Rect M{B.X + B.W - 410.0f, B.Y + 22.0f, 386.0f, 178.0f};
		UI.RRect(M, 14.0f, Rgba(4, 8, 16, 0.6f), NetA(Col, 0.5f));
		NetSpaced(*C, "MAIN EVENT \xC2\xB7 " + NetUpper(net::WeekdayName(net::DayOf(Main.Start))) + " " + NetUpper(net::DateLabel(net::DayOf(Main.Start))) + " \xC2\xB7 " + net::TimeLabel(Main.Start), M.X + 20.0f, M.Y + 30.0f, 10.5f, 800, Col, 1.5f);
		UI.Text(NetMoney(T.GtdCents), M.X + 20.0f, M.Y + 76.0f, Ts(38.0f, 900, pal::Gold));
		UI.Text(net::BuyIn(T.BuyInCents) + " buy-in \xC2\xB7 " + Grouped(T.StartStack) + " chips", M.X + 20.0f, M.Y + 100.0f, Ts(14.0f, 600, Hex(0xc3cedf)));
		if (L.St == net::Status::Finished)
		{
			const net::EventResult& Res = Net.Result(Main);
			if (!Res.FinalTable.empty())
			{
				const net::Placing& W = Res.FinalTable.front();
				NetTrophy(*C, M.X + 34.0f, M.Y + 118.0f, 40.0f, Hex(0xffe08a), Hex(0xc9962b));
				UI.Text(PlacingName(W), M.X + 64.0f, M.Y + 140.0f, Ts(17.0f, 800, pal::Ink));
				UI.Text("won " + Money(W.Prize), M.X + 64.0f, M.Y + 160.0f, Ts(13.0f, 600, pal::Gold));
			}
		}
		else
		{
			NetSpaced(*C, L.St == net::Status::Announced || L.St == net::Status::Registering ? "STARTS IN" : NetUpper(net::StatusName(L.St)), M.X + 20.0f, M.Y + 132.0f, 10.5f, 700, pal::Muted, 1.4f);
			UI.Text(L.StartsIn > 0.0 ? NetHms(L.StartsIn) : Grouped(L.Left) + " left", M.X + 20.0f, M.Y + 162.0f, Ts(24.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
		}
		ButtonOpts O;
		O.Size = 15.0f;
		if (UI.Button("mainev", {M.X + M.W - 130.0f, M.Y + M.H - 50.0f, 112.0f, 36.0f}, "Details", O))
		{
			SelectEvent(Main.Id);
			OpenPage(Page::Lobby, Now);
		}
	}

	// Calendar.
	const Rect Left{24.0f, 372.0f, 1000.0f, 578.0f};
	Section("CALENDAR", Left.X, Left.Y + 14.0f, Col);
	const float Gap = 6.0f;
	const float Cw = (Left.W - Gap * Nf(DayCount - 1)) / Nf(DayCount);
	for (int D = Sr->FirstDay; D <= Sr->LastDay; ++D)
	{
		const int K = D - Sr->FirstDay;
		const Rect R{Left.X + Nf(K) * (Cw + Gap), Left.Y + 30.0f, Cw, 72.0f};
		const float Ci = NetEase((Since - 0.02 * K) / 0.5);
		const Ui::ClickState St = UI.Clickable("day" + std::to_string(D), R);
		if (St.Clicked)
		{
			SeriesDay = D;
			SeriesAt = Now - 0.8;
		}
		const bool On = D == SeriesDay;
		const bool Past = D < Today;
		const bool IsToday = D == Today;
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ci);
		UI.RRect({R.X, R.Y + (1.0f - Ci) * 10.0f, R.W, R.H}, 10.0f, On ? NetA(Col, 0.22f) : St.Hover ? Hex(0x172a42) : Hex(0x101a2b), On || IsToday ? Col : Rgba(255, 255, 255, 0.05f), On ? 1.5f : 1.0f);
		const float Mx = R.X + R.W / 2.0f;
		const bool Narrow = R.W < 50.0f;
		UI.Text(Narrow ? std::string(net::WeekdayName(D)).substr(0, 1) : NetUpper(net::WeekdayName(D)), Mx, R.Y + 20.0f, Ts(10.5f, 800, IsToday ? Col : pal::Muted, Align::Center));
		const std::string Date = net::DateLabel(D);
		UI.Text(Date.substr(Date.find(' ') + 1), Mx, R.Y + 46.0f, Ts(Narrow ? 18.0f : 22.0f, 900, Past && !On ? pal::Dim : pal::Ink, Align::Center));
		if (Past)
		{
			NetCheck(*C, Mx, R.Y + 60.0f, 9.0f, NetA(pal::Green, 0.8f));
		}
		else if (IsToday)
		{
			NetSpaced(*C, Narrow ? "NOW" : "TODAY", Mx, R.Y + 64.0f, 8.5f, 900, Col, 0.8f, Align::Center);
		}
		else
		{
			C->FillCircle(Mx, R.Y + 60.0f, 2.5f, pal::Dim);
		}
		if (HasMain && net::DayOf(Main.Start) == D)
		{
			NetStar(*C, R.X + R.W - 9.0f, R.Y + 10.0f, 5.0f, pal::Gold);
		}
		C->SetAlpha(A0);
	}
	// The day's events.
	std::vector<net::EventInstance> DayEvents;
	for (net::EventInstance& E : Net.Window(static_cast<double>(SeriesDay) * net::MinutesPerDay, static_cast<double>(SeriesDay + 1) * net::MinutesPerDay))
	{
		if (Net.TemplateOf(E).Series == Sr->Id)
		{
			DayEvents.push_back(std::move(E));
		}
	}
	UI.Text(std::string(net::WeekdayName(SeriesDay, true)) + ", " + net::DateLabel(SeriesDay), Left.X, Left.Y + 136.0f, Ts(20.0f, 800, pal::Ink));
	UI.Text(std::to_string(DayEvents.size()) + " events", Left.X + Left.W, Left.Y + 136.0f, Ts(14.0f, 600, pal::Muted, Align::Right));
	for (size_t I = 0; I < DayEvents.size() && I < 7; ++I)
	{
		const net::EventInstance& E = DayEvents[I];
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, World);
		const float Ri = NetEase((Since - 0.15 - 0.05 * static_cast<double>(I)) / 0.45);
		const Rect R{Left.X + (1.0f - Ri) * 30.0f, Left.Y + 152.0f + Nf(I) * 60.0f, Left.W, 54.0f};
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		const Ui::ClickState St = UI.Clickable("sev" + E.Id, R);
		if (St.Clicked)
		{
			SelectEvent(E.Id);
			OpenPage(Page::Lobby, Now);
		}
		const bool Big = T.Featured;
		UI.RRect(R, 10.0f, St.Hover ? Hex(0x15253d) : Hex(0x101a2b), Big ? NetA(pal::Gold, 0.55f) : Rgba(255, 255, 255, 0.04f));
		C->FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, Big ? pal::Gold : Col);
		UI.Text(net::TimeLabel(E.Start), R.X + 20.0f, R.Y + 33.0f, Ts(15.0f, 700, pal::Muted));
		eventart::Emblem(*C, T, R.X + 122.0f, R.Y + R.H / 2.0f, 46.0f, Now);
		const float Nw = NetPill(*C, "#" + std::to_string(T.EventNo), R.X + 152.0f, R.Y + 18.0f, Big ? pal::Gold : Col, true, 10.0f);
		UI.Text(NetShortName(T.Name), R.X + 160.0f + Nw, R.Y + 33.0f, Ts(16.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, 300.0f));
		UI.Text(net::BuyIn(T.BuyInCents), R.X + 560.0f, R.Y + 33.0f, Ts(15.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
		UI.Text(NetMoney(std::max(L.Pool, T.GtdCents)) + (L.St == net::Status::Finished ? "" : " GTD"), R.X + 590.0f, R.Y + 33.0f, Ts(15.0f, 700, pal::Ink));
		if (L.St == net::Status::Finished)
		{
			const net::EventResult& Res = Net.Result(E);
			if (!Res.FinalTable.empty())
			{
				const net::Placing& W = Res.FinalTable.front();
				NetTrophy(*C, R.X + 744.0f, R.Y + 13.0f, 26.0f, Hex(0xffe08a), Hex(0xc9962b));
				UI.Text(PlacingName(W), R.X + 764.0f, R.Y + 27.0f, Ts(14.0f, 700, W.Player == -1 ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, 150.0f));
				UI.Text(Money(W.Prize), R.X + 764.0f, R.Y + 44.0f, Ts(12.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, true));
			}
		}
		else
		{
			const Color Sc = NetStatus(L.St);
			const std::string Label = L.St == net::Status::LateReg ? "LATE " + NetHms(L.LateRegLeft) : L.St == net::Status::Registering || L.St == net::Status::Announced ? "IN " + NetUpper(net::Countdown(L.StartsIn)) : NetUpper(net::StatusName(L.St));
			NetPill(*C, Label, R.X + 744.0f, R.Y + 17.0f, Sc, false, 10.5f);
		}
		NetChevron(*C, R.X + R.W - 20.0f, R.Y + R.H / 2.0f, 12.0f, 0, St.Hover ? pal::Ink : pal::Dim);
		C->SetAlpha(A0);
	}

	// Right column.
	const Rect Rt{1044.0f, 372.0f, 532.0f, 578.0f};
	Panel(Rt);
	if (!Ahead && !Over)
	{
		Section("SERIES LEADERBOARD", Rt.X + 24.0f, Rt.Y + 36.0f, Col);
		net::BoardRow Mine;
		const std::vector<net::BoardRow> Rows = Net.Leaderboard(net::Board::Series, World, You, 8, &Mine);
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			const net::BoardRow& Rw = Rows[I];
			const float Y = Rt.Y + 58.0f + Nf(I) * 50.0f;
			const float Ri = NetEase((Since - 0.2 - 0.06 * static_cast<double>(I)) / 0.5);
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * Ri);
			if (Rw.Player < 0)
			{
				UI.RRect({Rt.X + 14.0f, Y, Rt.W - 28.0f, 44.0f}, 8.0f, NetA(pal::Accent, 0.12f), NetA(pal::Accent, 0.5f));
			}
			const Color Medal = Rw.Rank == 1 ? pal::Gold : Rw.Rank == 2 ? Hex(0xc9d3e0) : Rw.Rank == 3 ? Hex(0xd08c4f) : pal::Muted;
			UI.Text(std::to_string(Rw.Rank), Rt.X + 44.0f, Y + 28.0f, Ts(16.0f, 900, Medal, Align::Right, Baseline::Alphabetic, true));
			const std::string& Nm = Rw.Player < 0 ? S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
			const AvatarSpec Pic = NetAvatar(*C, Rt.X + 76.0f, Y + 22.0f, 15.0f, Nm, Rw.Rank <= 3 ? Medal : Color{0.0f, 0.0f, 0.0f, 0.0f}, Rw.Player < 0);
			PlayerName(Rw.Player, Rt.X + 102.0f, Y + 28.0f, 15.0f, 260.0f, true, NetBadgeFrame(Pic));
			UI.Text(Grouped(static_cast<int64_t>(std::llround(Rw.Value))), Rt.X + Rt.W - 30.0f, Y + 28.0f, Ts(16.0f, 800, pal::Ink, Align::Right, Baseline::Alphabetic, true));
			C->SetAlpha(A0);
		}
		const float Fy = Rt.Y + Rt.H - 96.0f;
		C->FillRect({Rt.X + 24.0f, Fy, Rt.W - 48.0f, 1.0f}, pal::Line);
		UI.Text("You", Rt.X + 24.0f, Fy + 32.0f, Ts(14.0f, 600, pal::Muted));
		UI.Text(Mine.Rank > 0 ? "#" + Grouped(Mine.Rank) + " \xC2\xB7 " + Grouped(static_cast<int64_t>(std::llround(Mine.Value))) + " pts" : std::string("No points yet"), Rt.X + Rt.W - 24.0f, Fy + 32.0f,
			Ts(16.0f, 800, Mine.Rank > 0 ? pal::Accent : pal::Muted, Align::Right));
		const std::string Note = Sr->Id == "mm" ? "Every final table scores. The top three win RCOP Main Event packages worth $6,500."
			: Sr->Bracelets > 0 ? "Every cash scores. " + std::to_string(Sr->Bracelets) + " of the titles come with a Championship bracelet."
			: Sr->Rings > 0 ? "Every cash scores. " + std::to_string(Sr->Rings) + " of the titles come with a Grand Circuit ring."
							: "Every cash scores; the deeper the run and the bigger the field, the more it's worth.";
		NetParagraph(*C, Note, Rt.X + 24.0f, Fy + 60.0f, Rt.W - 48.0f, 13.0f, 500, pal::Muted, 18.0f, 2);
	}
	else if (Ahead && Sr->Id.rfind("rcop", 0) != 0)
	{
		// What to circle on the calendar: the Main Event, the high rollers, the ring and bracelet events.
		Section("HIGHLIGHTS", Rt.X + 24.0f, Rt.Y + 36.0f, Col);
		std::vector<const net::EventTemplate*> Picks;
		for (int D = Sr->FirstDay; D <= Sr->LastDay; ++D)
		{
			for (const net::EventInstance& E : Net.Window(static_cast<double>(D) * net::MinutesPerDay, static_cast<double>(D + 1) * net::MinutesPerDay))
			{
				const net::EventTemplate& T = Net.TemplateOf(E);
				if (T.Series == Sr->Id)
				{
					Picks.push_back(&T);
				}
			}
		}
		std::stable_sort(Picks.begin(), Picks.end(), [](const net::EventTemplate* Lhs, const net::EventTemplate* Rhs) { return Lhs->GtdCents > Rhs->GtdCents; });
		for (size_t I = 0; I < Picks.size() && I < 9; ++I)
		{
			const net::EventTemplate& T = *Picks[I];
			const float Y = Rt.Y + 58.0f + Nf(I) * 50.0f;
			const float Ri = NetEase((Since - 0.15 - 0.05 * static_cast<double>(I)) / 0.45);
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * Ri);
			UI.Text(NetUpper(net::DateLabel(T.OnlyDay)), Rt.X + 24.0f, Y + 26.0f, Ts(11.5f, 800, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			eventart::Emblem(*C, T, Rt.X + 108.0f, Y + 21.0f, 40.0f, Now);
			UI.Text(NetShortName(T.Name), Rt.X + 136.0f, Y + 26.0f, Ts(15.0f, 700, T.Main ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, Rt.W - 270.0f));
			if (T.Bracelet || T.Ring)
			{
				NetPill(*C, T.Bracelet ? "BRACELET" : "RING", Rt.X + 136.0f, Y + 34.0f, Hex(0xf2c14e), true, 8.5f);
			}
			UI.Text(NetMoney(T.GtdCents), Rt.X + Rt.W - 24.0f, Y + 26.0f, Ts(15.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
			C->SetAlpha(A0);
		}
	}
	else if (Ahead)
	{
		// The satellite ladder to the Main Event.
		Section("ROAD TO THE MAIN EVENT", Rt.X + 24.0f, Rt.Y + 36.0f, Col);
		const char* Ids[4] = {"step1", "step2", "step3", "step4"};
		const char* Labels[5] = {"STEP 1", "STEP 2", "STEP 3", "STEP 4", "MAIN"};
		const char* Buys[5] = {"$2.20", "$11", "$55", "$215", "$5,250"};
		const float Bw = (Rt.W - 48.0f - 4.0f * 8.0f) / 5.0f;
		const float Base = Rt.Y + 400.0f;
		for (int K = 0; K < 5; ++K)
		{
			const float H = (70.0f + Nf(K) * 58.0f) * NetEase((Since - 0.1 * K) / 0.6);
			const float X = Rt.X + 24.0f + Nf(K) * (Bw + 8.0f);
			const Color Kc = K == 4 ? pal::Gold : Mix(Col, Col2, Nf(K) / 4.0f);
			if (K == 4)
			{
				C->GlowRoundRect({X, Base - H, Bw, H}, 10.0f, NetA(pal::Gold, 0.4f), 20.0f);
			}
			C->FillRoundRect({X, Base - H, Bw, H}, 10.0f, Paint::Linear({0.0f, Base - H}, {0.0f, Base}, NetA(Kc, 0.95f), NetA(Kc, 0.18f)));
			NetSpaced(*C, Labels[K], X + Bw / 2.0f, Base - H + 20.0f, 9.5f, 900, Hex(0x07121c), 1.0f, Align::Center);
			UI.Text(Buys[K], X + Bw / 2.0f, Base - H + 42.0f, Ts(15.0f, 900, Hex(0x07121c), Align::Center));
			if (K < 4)
			{
				net::EventInstance Next;
				if (Net.Next(Ids[K], World, Next))
				{
					UI.Text(net::TimeLabel(Next.Start), X + Bw / 2.0f, Base + 20.0f, Ts(11.5f, 700, pal::Muted, Align::Center));
				}
			}
			else
			{
				NetTrophy(*C, X + Bw / 2.0f, Base - H - 58.0f, 48.0f, Hex(0xffe08a), Hex(0xc9962b));
				UI.Text(HasMain ? NetUpper(net::DateLabel(net::DayOf(Main.Start))) : std::string(), X + Bw / 2.0f, Base + 20.0f, Ts(11.5f, 700, pal::Gold, Align::Center));
			}
		}
		// You are here: the highest step the player holds a ticket for.
		int Here = 0;
		const char* Tickets[4] = {"step2", "step3", "step4", "rcop-main"};
		for (int K = 0; K < 4; ++K)
		{
			Here = S.Life.TicketsFor(Tickets[K]) > 0 ? K + 1 : Here;
		}
		const float Yx = Rt.X + 24.0f + Bw / 2.0f + Nf(Here) * (Bw + 8.0f);
		const float Yy = Base - (70.0f + Nf(Here) * 58.0f) - 30.0f + Nf(std::sin(Now * 3.0)) * 3.0f;
		NetPill(*C, "YOU \xC2\xB7 " + Money(S.BankrollCents), Yx - 38.0f, Yy - 24.0f, pal::Accent, true, 10.0f);
		NetTriangle(*C, Yx, Yy + 2.0f, 10.0f, false, pal::Accent);
		const float Cy = Base + 48.0f;
		UI.RRect({Rt.X + 24.0f, Cy, Rt.W - 48.0f, 118.0f}, 10.0f, Rgba(242, 193, 78, 0.07f), NetA(pal::Gold, 0.35f));
		UI.Text("$2.20 to a $5,250 seat.", Rt.X + 44.0f, Cy + 34.0f, Ts(20.0f, 900, pal::Ink));
		NetParagraph(*C, (S.Unlocks() & net::UnlockSatellite) ? "Five steps to the Main Event and its $25,000,000 guarantee. Last year's champion won $4,108,220. Step 1 runs every four hours."
															   : "Five steps to the Main Event and its $25,000,000 guarantee. Last year's champion won $4,108,220. Make a final table to unlock satellites.",
			Rt.X + 44.0f, Cy + 60.0f, Rt.W - 88.0f, 13.5f, 500, Hex(0xc9d3e2), 19.0f, 3);
	}
	else
	{
		Section("CHAMPIONS", Rt.X + 24.0f, Rt.Y + 36.0f, Col);
		if (HasMain)
		{
			const net::EventResult& Res = Net.Result(Main);
			UI.Text(Net.TemplateOf(Main).Name, Rt.X + 24.0f, Rt.Y + 76.0f, Ts(17.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, Rt.W - 48.0f));
			UI.Text(Grouped(Main.Entries) + " entries \xC2\xB7 " + NetMoney(Main.Pool) + " prize pool", Rt.X + 24.0f, Rt.Y + 98.0f, Ts(13.0f, 600, pal::Muted));
			for (size_t I = 0; I < Res.FinalTable.size(); ++I)
			{
				const net::Placing& P = Res.FinalTable[I];
				const float Y = Rt.Y + 116.0f + Nf(I) * 44.0f;
				UI.Text(Ordinal(P.Place), Rt.X + 24.0f, Y + 26.0f, Ts(14.0f, 800, P.Place == 1 ? pal::Gold : pal::Muted));
				PlayerName(P, Rt.X + 76.0f, Y + 26.0f, 15.0f, 260.0f, true);
				UI.Text(Money(P.Prize), Rt.X + Rt.W - 24.0f, Y + 26.0f, Ts(15.0f, 700, P.Place == 1 ? pal::Gold : pal::Ink, Align::Right, Baseline::Alphabetic, true));
			}
		}
	}
}

// ------------------------------------------------------------------ leaderboards

std::string RiverLine::BoardValue(net::Board B, double V) const
{
	const long long N = std::llround(V);
	switch (B)
	{
	case net::Board::Earnings: return NetMoney(static_cast<Chips>(N));
	case net::Board::Wins: return Grouped(static_cast<int64_t>(N)) + (N == 1 ? " title" : " titles");
	case net::Board::FinalTables: return Grouped(static_cast<int64_t>(N)) + " FTs";
	default: return Grouped(static_cast<int64_t>(N)) + " pts";
	}
}

void RiverLine::BoardsPage(double Now)
{
	const net::Network& Net = net::Shared();
	const net::Board Order[7] = {net::Board::NightShift, net::Board::Season, net::Board::Earnings, net::Board::Wins, net::Board::FinalTables, net::Board::Live, net::Board::Series};
	const net::SeriesInfo* Sr = Net.CurrentSeries(World);
	const bool SeriesLive = Sr && net::DayOf(World) >= Sr->FirstDay;
	auto Name = [&](net::Board B) { return B == net::Board::Series && Sr ? Sr->Name : std::string(NetBoardName(B)); };
	float X = 24.0f;
	for (net::Board B : Order)
	{
		if (B == net::Board::Series && !SeriesLive)
		{
			continue;
		}
		const std::string Label = B == net::Board::Live ? std::string("Live") : Name(B);
		const Rect R{X, 84.0f, UI.Measure(Label, 15.0f, 700) + 36.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable("board" + Label, R);
		if (St.Clicked)
		{
			ShowBoard(B, Now);
		}
		const bool On = BoardShown == B && !HonorsShown;
		if (On)
		{
			C->GlowRoundRect(R, 18.0f, NetA(pal::Accent, 0.3f), 10.0f);
			UI.RRect(R, 18.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(0x27d3c3), Hex(0x1fb3a5)));
		}
		else
		{
			UI.RRect(R, 18.0f, St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.02f), pal::Line);
		}
		UI.Text(Label, R.X + R.W / 2.0f, R.Y + 18.5f, Ts(15.0f, 700, On ? Hex(0x06201d) : St.Hover ? pal::Ink : Hex(0xa9b5c8), Align::Center, Baseline::Middle));
		X += R.W + 10.0f;
	}
	{
		// HONORS (§5.6): a mode of this page, not a board (the boards' arrays are fixed, World.h's RanksYesterday): the Hall
		// of Fame's classes and the Players of the Year, in gold.
		const Rect R{X, 84.0f, UI.Measure("Honors", 15.0f, 700) + 36.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable("boardHonors", R);
		if (St.Clicked && !HonorsShown)
		{
			HonorsShown = true;
			BoardAt = Now;
		}
		if (HonorsShown)
		{
			C->GlowRoundRect(R, 18.0f, NetA(pal::Gold, 0.3f), 10.0f);
			UI.RRect(R, 18.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(0xf2c14e), Hex(0xd9a636)));
		}
		else
		{
			UI.RRect(R, 18.0f, St.Hover ? Hex(0x2a2414) : Rgba(242, 193, 78, 0.05f), NetA(pal::Gold, 0.45f));
		}
		UI.Text("Honors", R.X + R.W / 2.0f, R.Y + 18.5f, Ts(15.0f, 700, HonorsShown ? Hex(0x231704) : St.Hover ? pal::Ink : pal::Gold, Align::Center, Baseline::Middle));
	}
	if (BoardShown == net::Board::Series && !SeriesLive)
	{
		BoardShown = net::Board::NightShift;
	}
	if (HonorsShown)
	{
		HonorsPanel({24.0f, 140.0f, 1052.0f, 810.0f}, Now);
		return;
	}
	net::BoardRow Mine;
	const std::vector<net::BoardRow> Rows = Net.Leaderboard(BoardShown, World, You, 60, &Mine);
	const double Since = Now - std::max(BoardAt, PageAt);

	// Podium: second, first, third.
	const float Base = 400.0f;
	const float Mid = 24.0f + 1052.0f / 2.0f;
	C->FillEllipse(Mid, Base - 90.0f, 420.0f, 150.0f, Paint::Radial({Mid, Base - 90.0f}, 0.0f, {Mid, Base - 90.0f}, 420.0f, Rgba(242, 193, 78, 0.12f), 0.5f, Rgba(39, 211, 195, 0.04f), Rgba(39, 211, 195, 0.0f)));
	const int Slots[3] = {1, 0, 2};
	const float Offsets[3] = {-250.0f, 0.0f, 250.0f};
	// The steps sit a little lower than they did, so each name has a rail of its badges under it (§5.6).
	const float Heights[3] = {118.0f, 90.0f, 70.0f};
	const Color Medals[3] = {Hex(0xf2c14e), Hex(0xc9d3e0), Hex(0xd08c4f)};
	for (int K = 0; K < 3; ++K)
	{
		const int Place = Slots[K];
		if (Place >= static_cast<int>(Rows.size()))
		{
			continue;
		}
		const net::BoardRow& Rw = Rows[static_cast<size_t>(Place)];
		const float Rise = NetEase((Since - 0.12 * (2 - Place)) / 0.8);
		const float H = Heights[Place] * Rise;
		const float Cx = Mid + Offsets[K];
		const Color M = Medals[Place];
		C->FillRoundRect({Cx - 100.0f, Base - H, 200.0f, H}, 10.0f, Paint::Linear({0.0f, Base - H}, {0.0f, Base}, NetA(M, 0.55f), NetA(M, 0.06f)));
		C->FillRect({Cx - 90.0f, Base - H + 1.0f, 180.0f, 2.0f}, NetA(M, 0.9f));
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Rise);
		UI.Text(std::to_string(Rw.Rank), Cx, Base - H + 50.0f, Ts(40.0f, 900, NetA(Hex(0xffffff), 0.92f), Align::Center));
		UI.Text(BoardValue(BoardShown, Rw.Value), Cx, Base - H + 76.0f, Ts(15.0f, 800, Hex(0xffffff), Align::Center, Baseline::Alphabetic, true));
		const float Ar = Place == 0 ? 34.0f : 29.0f;
		const float Ay = Base - H - Ar - 60.0f;
		const std::string& Nm = Rw.Player < 0 ? S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
		if (Place == 0)
		{
			C->GlowRoundRect({Cx - Ar, Ay - Ar, Ar * 2.0f, Ar * 2.0f}, Ar, NetA(M, 0.5f), 20.0f);
			NetCrown(*C, Cx, Ay - Ar - 4.0f, 26.0f, M);
		}
		const AvatarSpec Pic = NetAvatar(*C, Cx, Ay, Ar, Nm, M, Rw.Player < 0);
		if (Rw.Player >= 0)
		{
			const net::Player& P = Net.Players()[static_cast<size_t>(Rw.Player)];
			const float Nw = UI.Measure(P.Name, 16.0f, 800);
			NetFlag(*C, P.Country, Cx - Nw / 2.0f - 13.0f, Ay + Ar + 12.0f, 18.0f, 12.0f);
			UI.Text(P.Name, Cx + 11.0f, Ay + Ar + 23.0f, Ts(16.0f, 800, P.Rival ? Hex(0xd5b8ff) : pal::Ink, Align::Center));
		}
		else
		{
			UI.Text(Nm, Cx, Ay + Ar + 23.0f, Ts(16.0f, 800, pal::Accent, Align::Center));
		}
		// Up to three of their badges at 18 under the name, centred; what the avatar's frame says is left out (C2).
		const std::vector<badge::Badge> Rail = NetBadgeRow(NetBadgesOf(S, Rw.Player < 0 ? -1 : Rw.Player), badge::Network, NetBadgeFrame(Pic), 3);
		const float Gs = 18.0f;
		const float Gw = Nf(Rail.size()) * Gs + Nf(Rail.size() > 0 ? Rail.size() - 1 : 0) * 6.0f;
		for (size_t G = 0; G < Rail.size(); ++G)
		{
			const float Gx = Cx - Gw / 2.0f + Nf(G) * (Gs + 6.0f) + Gs / 2.0f;
			const float Gy = Ay + Ar + 41.0f;
			badgeart::Draw(*C, Rail[G], Gx, Gy, Gs, Now);
			if (UI.Hover({Gx - Gs / 2.0f - 2.0f, Gy - Gs / 2.0f - 2.0f, Gs + 4.0f, Gs + 4.0f}))
			{
				NetTip& Tip = NetTipOf(this);
				Tip.Up = true;
				Tip.B = Rail[G];
				Tip.Npc = Rw.Player < 0 ? -1 : Rw.Player;
				Tip.X = Gx;
				Tip.Y = Gy + Gs / 2.0f;
			}
		}
		C->SetAlpha(A0);
	}
	if (Rows.empty())
	{
		UI.Text("No scores yet tonight.", Mid, 260.0f, Ts(20.0f, 700, pal::Muted, Align::Center));
	}
	BoardTable({24.0f, 414.0f, 1052.0f, 456.0f}, Rows, Now);

	// Your standing.
	const Rect Hb{24.0f, 880.0f, 1052.0f, 70.0f};
	C->GlowRoundRect(Hb, 12.0f, NetA(pal::Accent, 0.2f), 16.0f);
	UI.RRect(Hb, 12.0f, Paint::Linear({Hb.X, 0.0f}, {Hb.X + Hb.W, 0.0f}, Mix(Hex(0x111c2e), pal::Accent, 0.22f), Hex(0x111c2e)), NetA(pal::Accent, 0.6f), 1.5f);
	NetSpaced(*C, "YOU", Hb.X + 22.0f, Hb.Y + 28.0f, 10.5f, 900, pal::Accent, 1.6f);
	UI.Text(Mine.Rank > 0 ? "#" + Grouped(Mine.Rank) : std::string("\xE2\x80\x94"), Hb.X + 22.0f, Hb.Y + 56.0f, Ts(24.0f, 900, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	NetAvatar(*C, Hb.X + 190.0f, Hb.Y + 35.0f, 20.0f, S.HeroName, pal::Accent, true);
	UI.Text(S.HeroName, Hb.X + 222.0f, Hb.Y + 42.0f, Ts(18.0f, 800, pal::Ink));
	std::string Hint;
	if (Mine.Rank == 0)
	{
		Hint = BoardShown == net::Board::NightShift ? "Cash a micro-stakes final table before 6 AM to get on the board." : "Make a final table to get on the board.";
	}
	else if (BoardShown == net::Board::NightShift && Mine.Rank <= 20)
	{
		Hint = "In the money: " + Money(Mine.Prize) + " if the night ended now.";
	}
	else if (BoardShown == net::Board::NightShift && Rows.size() >= 20)
	{
		Hint = Grouped(static_cast<int64_t>(std::llround(Rows[19].Value - Mine.Value + 1.0))) + " pts to the top 20 (the prize zone).";
	}
	else
	{
		Hint = "Ahead of " + Fixed(std::max(0.0, 100.0 * (1.0 - static_cast<double>(Mine.Rank) / 412000.0)), 1) + "% of RiverLine.";
	}
	UI.Text(Hint, Hb.X + 420.0f, Hb.Y + 41.0f, Ts(15.0f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, 440.0f));
	UI.Text(Mine.Rank > 0 ? BoardValue(BoardShown, Mine.Value) : std::string("No score"), Hb.X + Hb.W - 24.0f, Hb.Y + 43.0f, Ts(20.0f, 800, Mine.Rank > 0 ? pal::Accent : pal::Muted, Align::Right, Baseline::Alphabetic, true));

	// About the board.
	const Rect Rt{1092.0f, 84.0f, 484.0f, 866.0f};
	Panel(Rt);
	UI.Text(Name(BoardShown), Rt.X + 26.0f, Rt.Y + 54.0f, Ts(28.0f, 900, pal::Ink, Align::Left, Baseline::Alphabetic, false, Rt.W - 52.0f));
	std::string About;
	double Ends = -1.0;
	// The season boards run January to December (and start again).
	const int Year = world::YearOf(net::DayOf(World));
	const std::string Yr = std::to_string(Year);
	const double YearEnds = static_cast<double>(world::YearStart(Year + 1)) * net::MinutesPerDay;
	switch (BoardShown)
	{
	case net::Board::NightShift:
		About = "Tonight's micro-stakes race. Every final table in a $5.50-or-less event from 6 PM to 6 AM scores: bigger fields and deeper runs score more. The top 20 share $1,000.";
		Ends = life::NightShiftStart(World) + 12.0 * 60.0;
		break;
	case net::Board::Season:
		About = "Player of the Year " + Yr + ". Points from every final table since January. The top three win RCOP " + std::to_string(Year + 1) + " Platinum Passes worth $25,000.";
		Ends = YearEnds;
		break;
	case net::Board::Earnings: About = "All-time tournament winnings on RiverLine. Every cash counts, every buy-in forgotten."; break;
	case net::Board::Wins: About = "Tournament titles won in " + Yr + ". Anyone can run deep once."; Ends = YearEnds; break;
	case net::Board::FinalTables: About = "Final tables reached in " + Yr + ". The consistency board."; Ends = YearEnds; break;
	case net::Board::Series:
		About = "Points from every final table at " + (Sr ? Sr->Name : std::string("the series")) + ". The top three win RCOP Main Event packages.";
		Ends = Sr ? static_cast<double>(Sr->LastDay + 1) * net::MinutesPerDay : -1.0;
		break;
	case net::Board::Live:
		About = "Live Player of the Year " + Yr + ": points from every cash at a regional festival or bigger, from the River City Classic to the Grand Circuit and the Championship in Las Vegas. The Embercrest has its own monthly board.";
		Ends = YearEnds;
		break;
	}
	float Y = NetParagraph(*C, About, Rt.X + 26.0f, Rt.Y + 90.0f, Rt.W - 52.0f, 15.0f, 500, Hex(0xc3cedf), 22.0f, 5) + 12.0f;
	if (Ends > World)
	{
		NetSpaced(*C, "ENDS IN", Rt.X + 26.0f, Y, 10.5f, 800, pal::Muted, 1.6f);
		UI.Text(NetHms(Ends - World), Rt.X + 26.0f, Y + 38.0f, Ts(32.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
		Y += 62.0f;
	}
	C->FillRect({Rt.X + 26.0f, Y, Rt.W - 52.0f, 1.0f}, pal::Line);
	Y += 30.0f;
	if (BoardShown == net::Board::NightShift)
	{
		Section("PRIZES", Rt.X + 26.0f, Y, pal::Gold);
		for (int Rank = 1; Rank <= 11; ++Rank)
		{
			const float Col = Nf((Rank - 1) / 6);
			const float Ry = Y + 26.0f + Nf((Rank - 1) % 6) * 26.0f;
			const float Cx = Rt.X + 26.0f + Col * (Rt.W - 52.0f) / 2.0f;
			const float Cw = (Rt.W - 52.0f) / 2.0f - 16.0f;
			const bool Yours = Mine.Rank == Rank || (Rank == 11 && Mine.Rank >= 11 && Mine.Rank <= 20);
			UI.Text(Rank == 11 ? std::string("11th\xE2\x80\x93") + "20th" : Ordinal(Rank), Cx, Ry, Ts(14.0f, 700, Yours ? pal::Accent : pal::Muted));
			UI.Text(NetMoney(net::Network::NightShiftPrize(Rank)), Cx + Cw, Ry, Ts(14.0f, 800, Yours ? pal::Accent : pal::Gold, Align::Right, Baseline::Alphabetic, true));
		}
		Y += 26.0f * 7.0f + 12.0f;
	}
	else if (BoardShown == net::Board::Season || BoardShown == net::Board::Series)
	{
		Section("PRIZES", Rt.X + 26.0f, Y, pal::Gold);
		const char* Prizes[3] = {BoardShown == net::Board::Season ? "Platinum Pass + $10,000" : "RCOP Main package ($6,500)", BoardShown == net::Board::Season ? "Platinum Pass + $5,000" : "RCOP Main package ($6,500)",
			BoardShown == net::Board::Season ? "Platinum Pass" : "RCOP Main package ($6,500)"};
		for (int I = 0; I < 3; ++I)
		{
			UI.Text(Ordinal(I + 1), Rt.X + 26.0f, Y + 28.0f + Nf(I) * 26.0f, Ts(14.0f, 700, pal::Muted));
			UI.Text(Prizes[I], Rt.X + Rt.W - 26.0f, Y + 28.0f + Nf(I) * 26.0f, Ts(14.0f, 800, pal::Gold, Align::Right));
		}
		Y += 26.0f * 4.0f;
	}
	if (BoardShown == net::Board::Season || BoardShown == net::Board::Live)
	{
		// REIGNING (§5.6): last year's Player of the Year (online on the Season board, live on the Live board).
		const bool LiveBoard = BoardShown == net::Board::Live;
		const char* Crown = LiveBoard ? "Live Player of the Year" : "Player of the Year";
		const world::Honor* Last = nullptr;
		for (const world::Honor& Hn : S.Living().Honors())
		{
			Last = Hn.Title == Crown && Hn.Year == Year - 1 ? &Hn : Last;
		}
		Section("REIGNING", Rt.X + 26.0f, Y, pal::Gold);
		if (Last)
		{
			const std::string& Nm = Last->Npc == -1 ? S.HeroName : Last->Name;
			const AvatarSpec Pic = NetAvatar(*C, Rt.X + 42.0f, Y + 28.0f, 15.0f, Nm, pal::Gold, Last->Npc == -1);
			if (Last->Npc >= -1 && (Last->Npc == -1 || S.Living().Get(Last->Npc)))
			{
				PlayerName(Last->Npc, Rt.X + 66.0f, Y + 34.0f, 15.0f, 250.0f, true, NetBadgeFrame(Pic));
			}
			else
			{
				UI.Text(Nm, Rt.X + 66.0f, Y + 34.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, 250.0f));
			}
			UI.Text(std::to_string(Last->Year) + " \xC2\xB7 " + Grouped(Last->Entries) + " pts", Rt.X + Rt.W - 26.0f, Y + 34.0f, Ts(13.0f, 700, pal::Gold, Align::Right, Baseline::Alphabetic, true));
		}
		else
		{
			UI.Text("Crowned on New Year's Day. Nobody holds it yet.", Rt.X + 26.0f, Y + 32.0f, Ts(13.5f, 500, pal::Muted));
		}
		Y += 68.0f;
	}
	if (BoardShown == net::Board::Live)
	{
		// CIRCUIT CARDS (§5.3, §5.6): circuit points from regional festivals up; the weekly standings from the season's 13th
		// week hand out the cards (top 200, 50, 10), the best of the season kept. The cut lines as they stand, who holds
		// each, and where the player is.
		const world::World& W = S.Living();
		Section("CIRCUIT CARDS", Rt.X + 26.0f, Y, pal::Gold);
		struct CardRow
		{
			int Tier;
			const char* Name;
		};
		const CardRow Cards[3] = {{3, "Black Card"}, {2, "Gold Card"}, {1, "Circuit Card"}};
		int Holders[4] = {0, 0, 0, 0};
		for (const world::Npc& N : W.People())
		{
			for (int Tr = 1; Tr <= 3; ++Tr)
			{
				Holders[Tr] += N.CardBest >= Tr ? 1 : 0;
			}
		}
		for (int Tr = 1; Tr <= 3; ++Tr)
		{
			Holders[Tr] += W.HeroCardBest() >= Tr ? 1 : 0;
		}
		for (int I = 0; I < 3; ++I)
		{
			const float Ry = Y + 30.0f + Nf(I) * 28.0f;
			badge::Badge B;
			B.K = badge::Kind::CircuitCard;
			B.Tier = Cards[I].Tier;
			B.Year = Year;
			badgeart::Draw(*C, B, Rt.X + 36.0f, Ry - 5.0f, 18.0f, Now);
			UI.Text(Cards[I].Name, Rt.X + 54.0f, Ry, Ts(14.0f, 700, pal::Ink));
			UI.Text("Top " + std::to_string(badge::CardTop[Cards[I].Tier - 1]), Rt.X + 170.0f, Ry, Ts(13.0f, 600, pal::Muted));
			const double Cut = W.CardCutLine(Cards[I].Tier);
			UI.Text(Cut > 0.0 ? "cut " + Grouped(static_cast<int64_t>(std::llround(Cut))) : std::string("cut \xE2\x80\x94"), Rt.X + 250.0f, Ry, Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(std::to_string(Holders[Cards[I].Tier]) + (Holders[Cards[I].Tier] == 1 ? " holds it" : " hold it"), Rt.X + Rt.W - 26.0f, Ry, Ts(13.0f, 700, Holders[Cards[I].Tier] > 0 ? pal::Ink : pal::Dim, Align::Right));
		}
		// The player's road to a card.
		const int Today = net::DayOf(World);
		const double Points = W.HeroCircuitPoints();
		const int Rank = W.CircuitRank(-1);
		std::string Road;
		if (W.HeroCardBest() >= 1)
		{
			Road = "You hold the " + std::string(Cards[3 - std::min(3, W.HeroCardBest())].Name) + " this season.";
		}
		else if (Points <= 0.0)
		{
			Road = "No circuit points yet. They come from cashes at a regional festival or bigger.";
		}
		else
		{
			const double Cut = W.CardCutLine(1);
			const int Short = Cut > 0.0 ? static_cast<int>(std::ceil(std::max(0.0, Cut - Points + 1.0))) : 0;
			Road = Short > 0 ? Grouped(Short) + " circuit pts to your Card." : "#" + Grouped(Rank) + " in circuit points: inside the Card's cut.";
			if (world::SeasonWeek(Today) < badge::CardFromWeek)
			{
				Road += " Standings post on Mondays from week " + std::to_string(badge::CardFromWeek) + ".";
			}
		}
		// The next festival that counts.
		static int FestDay = -1;
		static std::string FestLine;
		if (FestDay != Today)
		{
			FestDay = Today;
			FestLine.clear();
			for (const world::LiveEvent& E : world::LiveFestivals(Today, 120))
			{
				if (world::CircuitEvent(E.Kind, false) && E.Start >= World)
				{
					const int In = net::DayOf(E.Start) - Today;
					FestLine = (E.City.empty() ? E.Name : E.City) + (In <= 0 ? " is today." : In == 1 ? " is tomorrow." : " is in " + std::to_string(In) + " days.");
					break;
				}
			}
		}
		Y = NetParagraph(*C, Road + (FestLine.empty() ? std::string() : " " + FestLine), Rt.X + 26.0f, Y + 30.0f + 3.0f * 28.0f + 4.0f, Rt.W - 52.0f, 13.0f, 600, Hex(0xc3cedf), 18.0f, 3);
	}
	// Your standings everywhere.
	Section("YOUR STANDINGS", Rt.X + 26.0f, std::max(Y, Rt.Y + Rt.H - 300.0f), pal::Accent);
	float Sy = std::max(Y, Rt.Y + Rt.H - 300.0f) + 14.0f;
	for (net::Board B : Order)
	{
		if (B == net::Board::Series && !SeriesLive)
		{
			continue;
		}
		net::BoardRow Row;
		Net.Leaderboard(B, World, You, 0, &Row);
		const Rect R{Rt.X + 14.0f, Sy, Rt.W - 28.0f, 40.0f};
		const Ui::ClickState St = UI.Clickable("stand" + std::to_string(static_cast<int>(B)), R);
		if (St.Clicked)
		{
			ShowBoard(B, Now);
		}
		if (St.Hover || B == BoardShown)
		{
			UI.RRect(R, 8.0f, B == BoardShown ? NetA(pal::Accent, 0.1f) : Rgba(255, 255, 255, 0.03f));
		}
		UI.Text(Name(B), R.X + 12.0f, R.Y + 26.0f, Ts(14.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, 190.0f));
		UI.Text(Row.Rank > 0 ? "#" + Grouped(Row.Rank) : std::string("\xE2\x80\x94"), R.X + 280.0f, R.Y + 26.0f, Ts(15.0f, 800, Row.Rank > 0 && Row.Rank <= 100 ? pal::Accent : pal::Ink, Align::Right, Baseline::Alphabetic, true));
		UI.Text(Row.Value > 0.0 ? BoardValue(B, Row.Value) : std::string(""), R.X + R.W - 12.0f, R.Y + 26.0f, Ts(14.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
		Sy += 42.0f;
	}
}

void RiverLine::BoardTable(const Rect& R, const std::vector<net::BoardRow>& Rows, double Now)
{
	const net::Network& Net = net::Shared();
	const bool Prizes = BoardShown == net::Board::NightShift;
	const char* ValueHead = BoardShown == net::Board::Earnings ? "EARNINGS" : BoardShown == net::Board::Wins ? "TITLES" : BoardShown == net::Board::FinalTables ? "FINAL TABLES" : "POINTS";
	UI.RRect({R.X, R.Y, R.W, 32.0f}, 8.0f, Hex(0x14223a));
	NetSpaced(*C, "RANK", R.X + 20.0f, R.Y + 21.0f, 10.5f, 800, pal::Muted, 1.2f);
	NetSpaced(*C, "PLAYER", R.X + 150.0f, R.Y + 21.0f, 10.5f, 800, pal::Muted, 1.2f);
	NetSpaced(*C, "FORM \xC2\xB7 8 WEEKS", R.X + 560.0f, R.Y + 21.0f, 10.5f, 800, pal::Muted, 1.2f);
	NetSpaced(*C, ValueHead, Prizes ? R.X + 880.0f : R.X + R.W - 24.0f, R.Y + 21.0f, 10.5f, 800, pal::Muted, 1.2f, Align::Right);
	if (Prizes)
	{
		NetSpaced(*C, "PRIZE", R.X + R.W - 24.0f, R.Y + 21.0f, 10.5f, 800, pal::Muted, 1.2f, Align::Right);
	}
	const Rect Area{R.X, R.Y + 38.0f, R.W, R.H - 38.0f};
	const float Pitch = 42.0f;
	const size_t First = std::min<size_t>(3, Rows.size());
	const float Content = Nf(Rows.size() - First) * Pitch;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (UI.Hover(Area) && UI.Ptr.Wheel != 0.0f)
	{
		// Positive is down (the hosts send the wheel as the web does): a row and a half a notch.
		BoardScrollGoal += (UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * Pitch * 1.5f;
	}
	BoardScrollGoal = std::min(std::max(BoardScrollGoal, 0.0f), MaxScroll);
	BoardScroll += (BoardScrollGoal - BoardScroll) * NetFollow(Dt, 14.0);
	const double Since = Now - std::max(BoardAt, PageAt);
	C->PushClip(Area);
	for (size_t I = First; I < Rows.size(); ++I)
	{
		const net::BoardRow& Rw = Rows[I];
		const float Y = Area.Y + Nf(I - First) * Pitch - BoardScroll;
		if (Y + Pitch < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		const float Ri = NetEase((Since - 0.3 - 0.035 * static_cast<double>(std::min<size_t>(I - First, 12))) / 0.45);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		const Rect Rr{Area.X + (1.0f - Ri) * 24.0f, Y, Area.W - 10.0f, Pitch - 4.0f};
		const bool Hero = Rw.Player < 0;
		UI.RRect(Rr, 8.0f, Hero ? NetA(pal::Accent, 0.14f) : (I % 2 == 0 ? Hex(0x101a2b) : Hex(0x0e1726)), Hero ? NetA(pal::Accent, 0.6f) : Rgba(255, 255, 255, 0.02f));
		const float Ty = Rr.Y + 25.0f;
		UI.Text(Grouped(Rw.Rank), Rr.X + 56.0f, Ty, Ts(15.0f, 800, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		if (Rw.Move != 0 && !Hero)
		{
			const bool Up = Rw.Move > 0;
			NetTriangle(*C, Rr.X + 80.0f, Ty - 5.0f, 9.0f, Up, Up ? pal::Green : pal::Red);
			UI.Text(std::to_string(std::abs(Rw.Move)), Rr.X + 90.0f, Ty, Ts(12.0f, 700, Up ? pal::Green : pal::Red, Align::Left, Baseline::Alphabetic, true));
		}
		else
		{
			C->FillRect({Rr.X + 75.0f, Ty - 5.0f, 10.0f, 2.0f}, pal::Dim);
		}
		const std::string& Nm = Hero ? S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
		const AvatarSpec Pic = NetAvatar(*C, Rr.X + 136.0f, Rr.Y + Rr.H / 2.0f, 14.0f, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, Hero);
		PlayerName(Rw.Player, Rr.X + 160.0f, Ty, 15.0f, 360.0f, true, NetBadgeFrame(Pic));
		if (!Hero)
		{
			NetSpark(*C, {Rr.X + 560.0f, Rr.Y + 9.0f, 120.0f, 20.0f}, Rw.Form, NetA(pal::Accent2, 0.9f));
		}
		UI.Text(BoardValue(BoardShown, Rw.Value), Prizes ? Rr.X + 870.0f : Rr.X + Rr.W - 14.0f, Ty, Ts(15.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		if (Prizes && Rw.Prize > 0)
		{
			UI.Text(NetMoney(Rw.Prize), Rr.X + Rr.W - 14.0f, Ty, Ts(15.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
		}
		C->SetAlpha(A0);
	}
	C->PopClip();
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		UI.RRect({Area.X + Area.W - 4.0f, Area.Y + (Area.H - Th) * BoardScroll / MaxScroll, 4.0f, Th}, 2.0f, Rgba(255, 255, 255, 0.2f));
	}
}

// ------------------------------------------------------------------ news

void RiverLine::NewsPage(double Now)
{
	const net::Network& Net = net::Shared();
	NewsSeenAt = World;
	const std::vector<net::NewsItem>& Items = NewsFeed();
	const double Since = Now - PageAt;
	auto Card = [&](const net::NewsItem& It, const Rect& R, bool Headline, int Index) {
		const float Ci = NetEase((Since - 0.06 * Index) / 0.5);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ci);
		C->Save();
		C->Translate(0.0f, (1.0f - Ci) * 18.0f);
		const Color Kc = NetKind(It.Kind);
		C->GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 14.0f, Rgba(0, 0, 0, 0.35f), 16.0f);
		UI.RRect(R, 14.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Mix(Hex(0x111c2e), Kc, Headline ? 0.2f : 0.08f), Hex(0x0e1726)), NetA(Kc, Headline ? 0.45f : 0.18f));
		C->PushClip(R);
		if (Headline)
		{
			C->FillEllipse(R.X + R.W - 180.0f, R.Y + R.H / 2.0f, 300.0f, 220.0f, Paint::Radial({R.X + R.W - 180.0f, R.Y + R.H / 2.0f}, 0.0f, {R.X + R.W - 180.0f, R.Y + R.H / 2.0f}, 300.0f, NetA(Kc, 0.25f), 0.5f, NetA(Kc, 0.06f), NetA(Kc, 0.0f)));
		}
		C->PopClip();
		const float Px = R.X + (Headline ? 30.0f : 22.0f);
		const float Tw = NetPill(*C, It.Tag, Px, R.Y + (Headline ? 26.0f : 20.0f), Kc, true, Headline ? 11.0f : 9.5f);
		UI.Text(NetAgo(World - It.At, It.At), Px + Tw + 10.0f, R.Y + (Headline ? 41.0f : 33.0f), Ts(12.5f, 600, pal::Muted));
		const float TextW = R.W - (Headline ? 360.0f : 44.0f);
		float Y = NetParagraph(*C, It.Title, Px, R.Y + (Headline ? 92.0f : 66.0f), TextW, Headline ? 30.0f : 17.0f, 900, pal::Ink, Headline ? 36.0f : 22.0f, 2);
		NetParagraph(*C, It.Body, Px, Y + (Headline ? 6.0f : 2.0f), TextW, Headline ? 16.0f : 13.5f, 500, Hex(0xb9c4d6), Headline ? 23.0f : 19.0f, Headline ? 3 : 2);
		if (It.Amount > 0)
		{
			if (Headline)
			{
				UI.Text(Money(It.Amount), R.X + R.W - 36.0f, R.Y + R.H / 2.0f + 20.0f, Ts(46.0f, 900, Kc, Align::Right, Baseline::Alphabetic, true));
				if (It.Player >= 0)
				{
					NetAvatar(*C, R.X + R.W - 80.0f, R.Y + 70.0f, 30.0f, Net.Players()[static_cast<size_t>(It.Player)].Name, Kc);
				}
			}
			else
			{
				UI.Text(Money(It.Amount), R.X + R.W - 22.0f, R.Y + 34.0f, Ts(16.0f, 900, Kc, Align::Right, Baseline::Alphabetic, true));
			}
		}
		if (!Headline && It.Player >= 0)
		{
			// The winner, as the lobby shows them.
			const net::Player& Who = Net.Players()[static_cast<size_t>(It.Player)];
			const float Wy = R.Y + R.H - 34.0f;
			NetAvatar(*C, Px + 16.0f, Wy, 16.0f, Who.Name, Color{0.0f, 0.0f, 0.0f, 0.0f});
			UI.Text(Who.Name, Px + 42.0f, Wy - 1.0f, Ts(14.0f, 800, pal::Ink, Align::Left, Baseline::Middle));
			const float Fx = Px + 50.0f + UI.Measure(Who.Name, 14.0f, 800);
			NetFlag(*C, Who.Country, Fx, Wy - 7.0f, 18.0f, 12.0f);
			if (Who.Pro)
			{
				NetPill(*C, "TEAM RIVERLINE", Fx + 26.0f, Wy - 9.0f, pal::Gold, false, 9.0f);
			}
		}
		C->Restore();
		C->SetAlpha(A0);
	};
	if (!Items.empty())
	{
		Card(Items[0], {24.0f, 84.0f, 1052.0f, 250.0f}, true, 0);
	}
	for (size_t I = 1; I < Items.size() && I < 7; ++I)
	{
		const size_t K = I - 1;
		Card(Items[I], {24.0f + Nf(K % 2) * 534.0f, 352.0f + Nf(K / 2) * 200.0f, 518.0f, 184.0f}, false, static_cast<int>(I));
	}

	// Coming up.
	const Rect Rt{1092.0f, 84.0f, 484.0f, 866.0f};
	Panel(Rt);
	Section("COMING UP", Rt.X + 26.0f, Rt.Y + 40.0f, pal::Gold);
	const std::vector<net::EventInstance> Next = Net.Upcoming(World, 8.0 * net::MinutesPerDay);
	float Y = Rt.Y + 60.0f;
	for (size_t I = 0; I < Next.size() && I < 7; ++I)
	{
		const net::EventInstance& E = Next[I];
		const net::EventTemplate& T = Net.TemplateOf(E);
		const Rect R{Rt.X + 14.0f, Y, Rt.W - 28.0f, 70.0f};
		const Ui::ClickState St = UI.Clickable("up" + E.Id, R);
		if (St.Clicked)
		{
			SelectEvent(E.Id);
			OpenPage(Page::Lobby, Now);
		}
		if (St.Hover)
		{
			UI.RRect(R, 10.0f, Rgba(255, 255, 255, 0.04f));
		}
		const Color Ec = NetEdge(Net, T);
		const Rect D{R.X + 10.0f, R.Y + 10.0f, 52.0f, 50.0f};
		UI.RRect(D, 9.0f, NetA(Ec, 0.16f), NetA(Ec, 0.5f));
		UI.Text(NetUpper(net::WeekdayName(net::DayOf(E.Start))), D.X + D.W / 2.0f, D.Y + 18.0f, Ts(10.5f, 800, Ec, Align::Center));
		const std::string Date = net::DateLabel(net::DayOf(E.Start));
		UI.Text(Date.substr(Date.find(' ') + 1), D.X + D.W / 2.0f, D.Y + 41.0f, Ts(20.0f, 900, pal::Ink, Align::Center));
		UI.Text(T.Name, R.X + 76.0f, R.Y + 30.0f, Ts(15.5f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 250.0f));
		UI.Text(net::BuyIn(T.BuyInCents) + " \xC2\xB7 " + (T.GtdCents > 0 ? NetMoney(T.GtdCents) + " GTD" : T.Seats) + " \xC2\xB7 " + net::TimeLabel(E.Start), R.X + 76.0f, R.Y + 51.0f, Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, 250.0f));
		UI.Text(net::Countdown(E.Start - World), R.X + R.W - 12.0f, R.Y + 40.0f, Ts(14.0f, 700, pal::Gold, Align::Right, Baseline::Alphabetic, true));
		Y += 74.0f;
	}
	Y += 16.0f;
	Section("SERIES CALENDAR", Rt.X + 26.0f, Y, pal::Accent);
	Y += 16.0f;
	for (const net::SeriesInfo& Sr : Net.Series())
	{
		const int Today = net::DayOf(World);
		const bool Live = Today >= Sr.FirstDay && Today <= Sr.LastDay;
		const Rect R{Rt.X + 14.0f, Y, Rt.W - 28.0f, 46.0f};
		const Ui::ClickState St = UI.Clickable("cal" + Sr.Id, R);
		if (St.Clicked)
		{
			ShowSeries(Sr.Id, Now);
			OpenPage(Page::Series, Now);
		}
		if (St.Hover)
		{
			UI.RRect(R, 10.0f, Rgba(255, 255, 255, 0.04f));
		}
		C->FillRoundRect({R.X + 10.0f, R.Y + 12.0f, 6.0f, 22.0f}, 3.0f, Paint::Linear({0.0f, R.Y + 12.0f}, {0.0f, R.Y + 34.0f}, Hex(Sr.Color), Hex(Sr.Color2)));
		UI.Text(Sr.Name, R.X + 28.0f, R.Y + 29.0f, Ts(15.0f, 800, Today > Sr.LastDay ? pal::Muted : pal::Ink));
		UI.Text(net::DateLabel(Sr.FirstDay) + " \xE2\x80\x93 " + net::DateLabel(Sr.LastDay), R.X + 210.0f, R.Y + 29.0f, Ts(13.0f, 600, pal::Muted));
		UI.Text(Live ? "LIVE" : Today > Sr.LastDay ? "DONE" : "IN " + NetUpper(net::Countdown(static_cast<double>(Sr.FirstDay) * net::MinutesPerDay - World)), R.X + R.W - 12.0f, R.Y + 29.0f,
			Ts(12.0f, 800, Live ? Hex(Sr.Color) : pal::Dim, Align::Right));
		Y += 48.0f;
	}
}

// ------------------------------------------------------------------ career

void RiverLine::CareerPage(double Now)
{
	const net::Network& Net = net::Shared();
	const double Since = Now - PageAt;
	const float In = NetEase(Since / 0.9);
	// Rank by lifetime winnings.
	struct CareerTier
	{
		const char* Name;
		Chips From;
		uint32_t Col;
	};
	const CareerTier Tiers[7] = {{"Rookie", 0, 0x7b8aa3}, {"Grinder", 500, 0x27d3c3}, {"Regular", 5000, 0x4f9bff}, {"Shark", 50000, 0x9b6bff}, {"High Roller", 500000, 0xf2c14e}, {"Pro", 5000000, 0xf28a3a},
		{"Legend", 50000000, 0xef4d5a}};
	int Lv = 0;
	for (int I = 0; I < 7; ++I)
	{
		Lv = You.Earnings >= Tiers[I].From ? I : Lv;
	}
	const Color Lc = Hex(Tiers[Lv].Col);

	// Profile.
	const Rect P{24.0f, 84.0f, 1052.0f, 196.0f};
	C->GlowRoundRect({P.X, P.Y + 8.0f, P.W, P.H}, 16.0f, Rgba(0, 0, 0, 0.4f), 20.0f);
	UI.RRect(P, 16.0f, Paint::Linear({P.X, 0.0f}, {P.X + P.W, 0.0f}, Mix(Hex(0x0c1626), Lc, 0.16f), Hex(0x0d1729)), NetA(Lc, 0.35f));
	NetAvatar(*C, P.X + 96.0f, P.Y + 98.0f, 58.0f, S.HeroName, Lc, true);
	UI.Text(S.HeroName, P.X + 176.0f, P.Y + 78.0f, Ts(38.0f, 900, pal::Ink));
	UI.Text("RiverLine member since 2021 \xC2\xB7 plays the night shift", P.X + 176.0f, P.Y + 106.0f, Ts(15.0f, 500, pal::Muted));
	const float Tw = NetPill(*C, NetUpper(Tiers[Lv].Name), P.X + 176.0f, P.Y + 124.0f, Lc, true, 11.0f);
	if (Lv < 6)
	{
		const Chips From = Tiers[Lv].From;
		const Chips To = Tiers[Lv + 1].From;
		const float Frac = Nf(Clamp01(static_cast<double>(You.Earnings - From) / static_cast<double>(To - From))) * In;
		const Rect Bar{P.X + 186.0f + Tw, P.Y + 131.0f, 250.0f, 8.0f};
		UI.RRect(Bar, 4.0f, Rgba(255, 255, 255, 0.08f));
		UI.RRect({Bar.X, Bar.Y, std::max(8.0f, Bar.W * Frac), Bar.H}, 4.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, Lc, Hex(Tiers[Lv + 1].Col)));
		UI.Text(Money(To - You.Earnings) + " in winnings to " + Tiers[Lv + 1].Name, P.X + 176.0f, P.Y + 170.0f, Ts(13.0f, 600, pal::Muted));
	}
	// The trophy case: the player's bracelets and rings, the latest first.
	if (const world::World* Wd = Net.Attached())
	{
		const std::vector<world::Award>& Won = Wd->HeroAwards();
		if (!Won.empty())
		{
			const size_t Shown = std::min<size_t>(Won.size(), 3);
			const float Cx0 = P.X + 572.0f;
			NetSpaced(*C, "TROPHY CASE", Cx0 - 26.0f, P.Y + 44.0f, 10.5f, 800, Hex(0xf28a3a), 1.6f);
			C->FillRoundRect({Cx0 - 30.0f, P.Y + 132.0f, Nf(Shown) * 56.0f + 4.0f, 5.0f}, 2.5f, Paint::Linear({Cx0, 0.0f}, {Cx0 + 170.0f, 0.0f}, Hex(0x6b4e16), Hex(0x3a2a0c)));
			for (size_t K = 0; K < Shown; ++K)
			{
				eventart::Trophy(*C, Won[Won.size() - 1 - K], Cx0 + Nf(K) * 56.0f, P.Y + 102.0f, 54.0f * (0.85f + 0.15f * In), Now + 0.7 * static_cast<double>(K));
			}
			// At Northside Pawn & Loan (realism spec §5.8, §7.4 P1): the piece is stamped IN PAWN or SOLD; the title stays.
			// The stamps go on after every piece is in the case, so a neighbor's sparkle never covers one.
			for (size_t K = 0; K < Shown; ++K)
			{
				shopart::DrawAwardStamp(*C, shopart::AwardStamp(S, Won[Won.size() - 1 - K]), Cx0 + Nf(K) * 56.0f, P.Y + 102.0f, 54.0f);
			}
			int Bracelets = 0;
			for (const world::Award& A : Won)
			{
				Bracelets += A.Ring ? 0 : 1;
			}
			const int Rings = static_cast<int>(Won.size()) - Bracelets;
			std::string Count = Bracelets > 0 ? std::to_string(Bracelets) + (Bracelets == 1 ? " bracelet" : " bracelets") : std::string();
			Count += Rings > 0 ? (Count.empty() ? "" : " \xC2\xB7 ") + std::to_string(Rings) + (Rings == 1 ? " ring" : " rings") : "";
			UI.Text(Count, Cx0 - 26.0f, P.Y + 162.0f, Ts(13.0f, 700, pal::Gold));
		}
	}
	// Bankroll and the rent.
	const float Bx = P.X + P.W - 30.0f;
	NetSpaced(*C, "BANKROLL", Bx, P.Y + 44.0f, 10.5f, 800, pal::Muted, 1.6f, Align::Right);
	UI.Text(Money(static_cast<Chips>(static_cast<double>(S.BankrollCents) * static_cast<double>(In))), Bx, P.Y + 92.0f, Ts(44.0f, 900, pal::Gold, Align::Right, Baseline::Alphabetic, true));
	const life::State& Lf = S.Life;
	const Chips Rent = std::max<Chips>(1, Lf.RentDueCents);
	const float RentFrac = Nf(Clamp01(static_cast<double>(S.BankrollCents) / static_cast<double>(Rent)));
	const Rect Rb{Bx - 300.0f, P.Y + 128.0f, 300.0f, 8.0f};
	const bool Evicted = Lf.RentStage == life::Rent::Evicted;
	const std::string RentHead = Evicted ? "EVICTED \xC2\xB7 ON DEE'S COUCH"
		: Lf.RentStage == life::Rent::FinalNotice ? "FINAL NOTICE \xC2\xB7 " + NetUpper(net::Countdown(Lf.RentDeadline - World)) + " LEFT"
		: Lf.RentStage == life::Rent::Paid ? "RENT PAID \xC2\xB7 NEXT DUE " + NetUpper(net::DateLabel(net::DayOf(Lf.RentDeadline - 1.0)))
		: "RENT DUE " + NetUpper(net::WeekdayName(net::DayOf(Lf.RentDeadline - 1.0), true)) + " \xC2\xB7 " + NetUpper(net::Countdown(Lf.RentDeadline - World));
	NetSpaced(*C, RentHead, Rb.X, P.Y + 120.0f, 10.0f, 800, Lf.RentStage == life::Rent::Paid ? pal::Green : pal::Red, 1.4f);
	// (Evicted, the bar is the way back: the back rent and a month up front.)
	UI.RRect(Rb, 4.0f, Rgba(255, 255, 255, 0.08f));
	UI.RRect({Rb.X, Rb.Y, std::max(8.0f, Rb.W * RentFrac * In), Rb.H}, 4.0f, Paint::Linear({Rb.X, 0.0f}, {Rb.X + Rb.W, 0.0f}, pal::Red, pal::Gold));
	const std::string RentNote = S.BankrollCents >= Rent ? (Evicted ? "Enough to move back in: the Bank app." : "Covered: pay it in the Bank app.")
														 : Money(Rent - S.BankrollCents) + (Evicted ? " to go to move back in" : " to go of " + Money(Rent));
	UI.Text(RentNote, Bx, P.Y + 160.0f, Ts(13.0f, 600, S.BankrollCents >= Rent ? pal::Green : Hex(0xc3cedf), Align::Right));

	// Numbers.
	const int Cashes = You.Cashes;
	const std::pair<std::string, std::string> Tiles[6] = {
		{std::to_string(static_cast<int>(std::round(Nf(You.Tournaments) * In))), "TOURNAMENTS"},
		{std::to_string(static_cast<int>(std::round(Nf(Cashes) * In))), You.Tournaments > 0 ? "CASHES \xC2\xB7 " + Fixed(100.0 * Cashes / You.Tournaments, 0) + "% ITM" : std::string("CASHES")},
		{std::to_string(static_cast<int>(std::round(Nf(You.FinalTables) * In))), "FINAL TABLES"},
		{std::to_string(static_cast<int>(std::round(Nf(You.Wins) * In))), "TITLES"},
		{Money(static_cast<Chips>(static_cast<double>(You.Earnings) * static_cast<double>(In))), "WINNINGS"},
		{You.Best > 0 ? Money(You.Best) : std::string("\xE2\x80\x94"), "BEST SCORE"},
	};
	const float TileW = (1052.0f - 5.0f * 12.0f) / 6.0f;
	for (int I = 0; I < 6; ++I)
	{
		const Rect R{24.0f + Nf(I) * (TileW + 12.0f), 296.0f, TileW, 88.0f};
		Panel(R, 12.0f);
		UI.Text(Tiles[I].first, R.X + 18.0f, R.Y + 44.0f, Ts(I >= 4 ? 22.0f : 28.0f, 900, I == 3 && You.Wins > 0 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 30.0f));
		NetSpaced(*C, Tiles[I].second, R.X + 18.0f, R.Y + 70.0f, 9.5f, 800, pal::Muted, 1.2f);
	}

	// Your badges (next_segment_spec §5.6): the held rail beside the heading, then the next ones within reach, each an
	// honest count. The sponsors' read on the player along the right (§3.3, Q16: the words, never the thresholds).
	CareerBadges(*C, UI, this, S, Now, In);

	// Standings.
	Section("LEADERBOARD STANDINGS", 24.0f, 536.0f, pal::Accent);
	const net::Board Boards[5] = {net::Board::NightShift, net::Board::Season, net::Board::Earnings, net::Board::Wins, net::Board::Series};
	const float Sw = (1052.0f - 4.0f * 12.0f) / 5.0f;
	int NightRank = 0;
	for (int I = 0; I < 5; ++I)
	{
		net::BoardRow Row;
		Net.Leaderboard(Boards[I], World, You, 0, &Row);
		NightRank = Boards[I] == net::Board::NightShift ? Row.Rank : NightRank;
		const Rect R{24.0f + Nf(I) * (Sw + 12.0f), 548.0f, Sw, 86.0f};
		const Ui::ClickState St = UI.Clickable("cstand" + std::to_string(I), R);
		if (St.Clicked)
		{
			ShowBoard(Boards[I], Now);
			OpenPage(Page::Leaderboards, Now);
		}
		UI.RRect(R, 12.0f, St.Hover ? Hex(0x15253d) : Hex(0x101a2b), St.Hover ? Hex(0x2a3f60) : pal::Line);
		UI.Text(NetBoardName(Boards[I]), R.X + 16.0f, R.Y + 26.0f, Ts(13.0f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
		UI.Text(Row.Rank > 0 ? "#" + Grouped(Row.Rank) : std::string("Unranked"), R.X + 16.0f, R.Y + 58.0f, Ts(Row.Rank > 0 ? 24.0f : 18.0f, 900, Row.Rank > 0 && Row.Rank <= 100 ? pal::Accent : Row.Rank > 0 ? pal::Ink : pal::Dim));
		if (Row.Value > 0.0)
		{
			UI.Text(BoardValue(Boards[I], Row.Value), R.X + 16.0f, R.Y + 76.0f, Ts(11.5f, 600, pal::Muted));
		}
	}

	// Recent results.
	Section("RECENT TOURNAMENTS", 24.0f, 664.0f, pal::Accent);
	{
		// RiverLine Stats: the player's own dashboard.
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 13.0f;
		O.Enabled = !CardOpen();
		if (UI.Button("careerstats", {1076.0f - 168.0f, 640.0f, 168.0f, 32.0f}, "Your stats  \xE2\x80\xBA", O))
		{
			ShowPlayer(HeroCard, Now);
		}
	}
	if (S.History.empty())
	{
		UI.Text("No tournaments yet. The Night Owl Turbo is running right now.", 24.0f, 706.0f, Ts(16.0f, 500, pal::Muted));
	}
	for (size_t I = 0; I < S.History.size() && I < 5; ++I)
	{
		const HistoryEntry& H = S.History[I];
		const Rect R{24.0f, 680.0f + Nf(I) * 54.0f, 1052.0f, 48.0f};
		UI.RRect(R, 10.0f, Hex(0x101a2b), Rgba(255, 255, 255, 0.03f));
		const bool Cashed = H.Prize > 0;
		C->FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, H.Place == 1 ? pal::Gold : Cashed ? pal::Green : pal::Dim);
		UI.Text(H.Name, R.X + 22.0f, R.Y + 30.0f, Ts(16.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, 420.0f));
		UI.Text(Ordinal(H.Place) + " of " + Grouped(H.Entrants), R.X + 500.0f, R.Y + 30.0f, Ts(15.0f, 700, H.Place <= 9 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, true));
		UI.Text(Cashed ? "+" + Money(H.Prize) : std::string("\xE2\x80\x94"), R.X + 760.0f, R.Y + 30.0f, Ts(15.0f, 800, Cashed ? pal::Green : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		const float Acc = Nf(Clamp01(H.AccuracyPct / 100.0));
		UI.RRect({R.X + 820.0f, R.Y + 22.0f, 120.0f, 6.0f}, 3.0f, Rgba(255, 255, 255, 0.07f));
		UI.RRect({R.X + 820.0f, R.Y + 22.0f, std::max(6.0f, 120.0f * Acc * In), 6.0f}, 3.0f, Acc > 0.85f ? pal::Green : Acc > 0.7f ? pal::Gold : pal::Orange);
		UI.Text(Fixed(H.AccuracyPct, 0) + "%", R.X + R.W - 18.0f, R.Y + 30.0f, Ts(14.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
	}

	// Career path.
	const Rect Rt{1092.0f, 84.0f, 484.0f, 560.0f};
	Panel(Rt);
	struct Step
	{
		const char* Title;
		std::string Desc;
		bool Done;
		bool Locked;
	};
	const bool Satellites = (S.Unlocks() & net::UnlockSatellite) != 0;
	bool NightMoney = NightRank > 0 && NightRank <= 20;
	for (const life::LedgerEntry& E : S.Life.Ledger)
	{
		NightMoney = NightMoney || (E.Kind == 4 && E.Label.rfind("Night Shift", 0) == 0);
	}
	const std::vector<Step> Path = {
		{"First cash", "Finish in the money. Unlocks bounty tournaments.", You.Cashes > 0, false},
		{"Final table", "Make the last nine. Unlocks satellites.", You.FinalTables > 0, false},
		{"Champion", "Win a tournament. Unlocks six-max.", You.Wins > 0, false},
		{"Night Shift top 20", "Finish a night in the leaderboard's money.", NightMoney, false},
		{"Series title", "Win an event in any RiverLine series.", You.SeriesTitles > 0, false},
		{"Make rent", "Pay the $1,225 before Friday midnight.", S.Life.RentsPaid > 0, false},
		{"Road to the Main", "Win a $5,250 seat to the RCOP Main Event.", S.Life.TicketsFor("rcop-main") > 0, !Satellites},
		{"Life changer", "Win the RCOP Main Event. Last year: $4,108,220.", false, true},
	};
	int DoneCount = 0;
	for (const Step& St : Path)
	{
		DoneCount += St.Done ? 1 : 0;
	}
	Section("CAREER PATH", Rt.X + 26.0f, Rt.Y + 38.0f, pal::Gold);
	UI.Text(std::to_string(DoneCount) + " / " + std::to_string(Path.size()), Rt.X + Rt.W - 26.0f, Rt.Y + 38.0f, Ts(14.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
	bool CurrentShown = false;
	for (size_t I = 0; I < Path.size(); ++I)
	{
		const Step& St = Path[I];
		const float Y = Rt.Y + 62.0f + Nf(I) * 60.0f;
		const float Cx = Rt.X + 44.0f;
		if (I + 1 < Path.size())
		{
			C->FillRect({Cx - 1.0f, Y + 34.0f, 2.0f, 34.0f}, St.Done ? NetA(pal::Accent, 0.6f) : pal::Line);
		}
		const bool Current = !St.Done && !St.Locked && !CurrentShown;
		CurrentShown = CurrentShown || Current;
		if (St.Done)
		{
			C->FillCircle(Cx, Y + 18.0f, 14.0f, pal::Accent);
			NetCheck(*C, Cx, Y + 18.0f, 14.0f, Hex(0x06201d));
		}
		else if (St.Locked)
		{
			C->StrokeEllipse(Cx, Y + 18.0f, 14.0f, 14.0f, pal::Line, 2.0f);
			NetLockIcon(*C, Cx - 8.0f, Y + 9.0f, 16.0f, pal::Dim);
		}
		else if (Current)
		{
			const float Pulse = 0.5f + 0.5f * Nf(std::sin(Now * 3.0));
			C->FillCircle(Cx, Y + 18.0f, 14.0f + 5.0f * Pulse, NetA(pal::Gold, 0.18f * (1.0f - Pulse)));
			C->StrokeEllipse(Cx, Y + 18.0f, 14.0f, 14.0f, pal::Gold, 2.5f);
		}
		else
		{
			C->StrokeEllipse(Cx, Y + 18.0f, 14.0f, 14.0f, pal::Line, 2.0f);
		}
		UI.Text(St.Title, Cx + 30.0f, Y + 16.0f, Ts(16.0f, 800, St.Done ? pal::Ink : Current ? pal::Gold : St.Locked ? pal::Dim : Hex(0xc3cedf)));
		UI.Text(St.Desc, Cx + 30.0f, Y + 36.0f, Ts(13.0f, 500, St.Locked ? pal::Dim : pal::Muted, Align::Left, Baseline::Alphabetic, false, Rt.W - 100.0f));
	}

	// The rival.
	const net::Player& Rv = Net.Players()[static_cast<size_t>(Net.RivalIndex())];
	const Rect Rc{1092.0f, 660.0f, 484.0f, 290.0f};
	C->GlowRoundRect({Rc.X, Rc.Y + 6.0f, Rc.W, Rc.H}, 14.0f, Rgba(0, 0, 0, 0.35f), 16.0f);
	UI.RRect(Rc, 14.0f, Paint::Linear({Rc.X, Rc.Y}, {Rc.X + Rc.W, Rc.Y + Rc.H}, Hex(0x1d1233), Hex(0x0d1729)), NetA(Hex(0xb36bff), 0.45f));
	NetSpaced(*C, "YOUR RIVAL", Rc.X + 26.0f, Rc.Y + 36.0f, 10.5f, 900, Hex(0xb36bff), 1.8f);
	NetAvatar(*C, Rc.X + 60.0f, Rc.Y + 84.0f, 32.0f, Rv.Name, Hex(0xb36bff));
	UI.Text(Rv.Name, Rc.X + 108.0f, Rc.Y + 82.0f, Ts(24.0f, 900, Hex(0xe7dbff)));
	NetFlag(*C, Rv.Country, Rc.X + 108.0f, Rc.Y + 94.0f, 18.0f, 12.0f);
	const std::string Level = Rv.Stake == net::Tier::High ? "High-stakes" : Rv.Stake == net::Tier::Mid ? "Mid-stakes" : "Low-stakes";
	UI.Text(Level + " crusher \xC2\xB7 " + std::to_string(Rv.Wins) + " titles \xC2\xB7 never logs off", Rc.X + 134.0f, Rc.Y + 105.0f, Ts(13.0f, 600, pal::Muted));
	// The rival's card, like anyone's.
	if (UI.Clickable("rivalcard", {Rc.X + 20.0f, Rc.Y + 40.0f, Rc.W - 40.0f, 80.0f}, !CardOpen()).Clicked)
	{
		ShowPlayer(Net.RivalIndex(), LastFrame);
	}
	const std::pair<const char*, std::pair<double, double>> Versus[3] = {
		{"Winnings", {static_cast<double>(You.Earnings), static_cast<double>(Rv.Earnings)}},
		{"Titles", {static_cast<double>(You.Wins), static_cast<double>(Rv.Wins)}},
		{"Best score", {static_cast<double>(You.Best), static_cast<double>(Rv.Best)}},
	};
	for (int I = 0; I < 3; ++I)
	{
		const float Y = Rc.Y + 140.0f + Nf(I) * 46.0f;
		const double Mine = Versus[I].second.first;
		const double Theirs = Versus[I].second.second;
		const float Share = Nf((Mine + Theirs) > 0.0 ? Mine / (Mine + Theirs) : 0.5);
		UI.Text(Versus[I].first, Rc.X + Rc.W / 2.0f, Y + 12.0f, Ts(12.0f, 700, pal::Muted, Align::Center));
		const std::string L = I == 1 ? Grouped(static_cast<int64_t>(Mine)) : Money(static_cast<Chips>(Mine));
		const std::string R = I == 1 ? Grouped(static_cast<int64_t>(Theirs)) : Money(static_cast<Chips>(Theirs));
		UI.Text(L, Rc.X + 26.0f, Y + 12.0f, Ts(13.0f, 800, pal::Accent, Align::Left, Baseline::Alphabetic, true));
		UI.Text(R, Rc.X + Rc.W - 26.0f, Y + 12.0f, Ts(13.0f, 800, Hex(0xd5b8ff), Align::Right, Baseline::Alphabetic, true));
		const Rect Bar{Rc.X + 26.0f, Y + 22.0f, Rc.W - 52.0f, 6.0f};
		UI.RRect(Bar, 3.0f, NetA(Hex(0xb36bff), 0.6f));
		UI.RRect({Bar.X, Bar.Y, std::max(6.0f, Bar.W * Share * In), Bar.H}, 3.0f, pal::Accent);
	}
}
} // namespace ui
} // namespace ss
