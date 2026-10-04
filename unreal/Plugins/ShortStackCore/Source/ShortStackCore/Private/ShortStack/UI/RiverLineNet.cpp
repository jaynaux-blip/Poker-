// RiverLine's network lobby: the schedule, the series, the leaderboards, the news and the player's career.
// Everything here reads the simulated network (ShortStack/Game/Network.h) at the session's world clock.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

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
	}
	PageShown = P;
	S.ConfirmRegister = false;
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
	if (B != BoardShown)
	{
		BoardAt = Now;
		BoardScroll = BoardScrollGoal = 0.0f;
	}
	BoardShown = B;
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
	const char* Names[5] = {"Lobby", "Series", "Leaderboards", "News", "Career"};
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
	for (int I = 0; I < 5; ++I)
	{
		const float Tw = UI.Measure(Names[I], 17.0f, 700);
		const Ui::ClickState St = UI.Clickable(std::string("nav") + Names[I], {X - 12.0f, 10.0f, Tw + 24.0f, NetTop - 12.0f}, Navigable);
		if (St.Clicked)
		{
			S.ShowLobby();
			OpenPage(static_cast<Page>(I), Now);
		}
		const bool Active = InLobby && PageShown == static_cast<Page>(I);
		const Color Col = !Navigable ? pal::Dim : Active ? pal::Ink : St.Hover ? Hex(0xc3cedf) : InLobby ? pal::Muted : pal::Dim;
		UI.Text(Names[I], X, 40.0f, Ts(17.0f, Active ? 700 : 500, Col));
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
		X += Tw + 34.0f + Extra;
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

void RiverLine::PlayerName(const net::Placing& P, float X, float Y, float Size, float MaxW, bool Badges)
{
	if (P.Player != -2)
	{
		PlayerName(P.Player, X, Y, Size, MaxW, Badges);
		return;
	}
	// Someone nobody follows: just a name and a flag.
	NetFlag(*C, P.Country, X, Y - Size * 0.72f, Size * 1.2f, Size * 0.8f);
	UI.Text(P.Name, X + Size * 1.2f + 8.0f, Y, Ts(Size, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, MaxW - Size * 1.2f - 8.0f));
}

void RiverLine::PlayerName(int Index, float X, float Y, float Size, float MaxW, bool Badges)
{
	const net::Network& Net = net::Shared();
	if (Index < 0)
	{
		const float W = UI.Text(S.HeroName, X, Y, Ts(Size, 800, pal::Accent, Align::Left, Baseline::Alphabetic, false, MaxW));
		if (Badges)
		{
			NetPill(*C, "YOU", X + W + 8.0f, Y - Size * 0.78f, pal::Accent, true, 9.0f);
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
	if (Badges && P.Rival)
	{
		NetPill(*C, "RIVAL", Nx + W + 8.0f, Y - Size * 0.78f, Hex(0xb36bff), true, 9.0f);
	}
	else if (Badges && P.Pro)
	{
		NetPill(*C, "TEAM RL", Nx + W + 8.0f, Y - Size * 0.78f, pal::Gold, false, 9.0f);
	}
	else if (Badges)
	{
		// New on RiverLine this month: the kind of name that catches the eye.
		const world::Npc* N = S.Living().Get(Index);
		if (N && N->Came != world::Arrival::None && net::DayOf(World) - N->Arrived <= 30)
		{
			NetPill(*C, "NEW", Nx + W + 8.0f, Y - Size * 0.78f, pal::Accent, true, 9.0f);
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
		ListScrollGoal -= UI.Ptr.Wheel * Pitch * 1.5f;
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
	const bool Affordable = S.BankrollCents >= T.BuyInCents || Tickets > 0;
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
			NetAvatar(*C, Body.X + 12.0f, Y + 13.0f, 11.0f, N->Name, Color{0.0f, 0.0f, 0.0f, 0.0f});
			PlayerName(Who[I], Body.X + 32.0f, Y + 18.0f, 14.0f, Body.W - 230.0f, true);
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
			NetAvatar(*C, Body.X + 40.0f, Y + 13.0f, 11.0f, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, P.Player == -1);
			PlayerName(P, Body.X + 60.0f, Y + 18.0f, 14.0f, Body.W - 170.0f, true);
			if (Finished)
			{
				UI.Text(Money(P.Prize), Body.X + Body.W, Y + 18.0f, Ts(14.0f, 700, P.Place == 1 ? pal::Gold : pal::Ink, Align::Right, Baseline::Alphabetic, true));
			}
		}
	}
	C->SetAlpha(A0);
	RegisterBlock(E, L, {R.X + 26.0f, R.Y + R.H - 96.0f, R.W - 52.0f, 70.0f}, Now);
}

void RiverLine::RegisterBlock(const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now)
{
	const net::Network& Net = net::Shared();
	const net::EventTemplate& T = Net.TemplateOf(E);
	std::string Lock;
	const bool Joinable = Net.Joinable(E, &Lock, S.Unlocks());
	const int Tickets = S.Life.TicketsFor(Net.TicketOf(T.Id));
	const bool Affordable = S.BankrollCents >= T.BuyInCents || Tickets > 0;
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
		if (UI.Button("reg", R, Tickets > 0 ? std::string("Register with ticket") : "Register \xC2\xB7 " + net::BuyIn(T.BuyInCents), O))
		{
			S.ConfirmRegister = true;
		}
	}
	else
	{
		UI.Text(Tickets > 0 ? std::string("One ticket will be used") : "Balance after: " + Money(S.BankrollCents - T.BuyInCents), R.X, R.Y - 10.0f, Ts(14.0f, 600, pal::Muted));
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
	}
	C->Restore();
	C->SetAlpha(A0);
	if (Card)
	{
		UI.Ptr = Real;
		PlayerCard(Now);
	}
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
			NetAvatar(*C, Rt.X + 76.0f, Y + 22.0f, 15.0f, Nm, Rw.Rank <= 3 ? Medal : Color{0.0f, 0.0f, 0.0f, 0.0f}, Rw.Player < 0);
			PlayerName(Rw.Player, Rt.X + 102.0f, Y + 28.0f, 15.0f, 260.0f, true);
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
		const bool On = BoardShown == B;
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
	if (BoardShown == net::Board::Series && !SeriesLive)
	{
		BoardShown = net::Board::NightShift;
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
	const float Heights[3] = {128.0f, 98.0f, 76.0f};
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
		const float Ay = Base - H - Ar - 40.0f;
		const std::string& Nm = Rw.Player < 0 ? S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
		if (Place == 0)
		{
			C->GlowRoundRect({Cx - Ar, Ay - Ar, Ar * 2.0f, Ar * 2.0f}, Ar, NetA(M, 0.5f), 20.0f);
			NetCrown(*C, Cx, Ay - Ar - 4.0f, 26.0f, M);
		}
		NetAvatar(*C, Cx, Ay, Ar, Nm, M, Rw.Player < 0);
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
		About = "Live Player of the Year " + Yr + ": points from every live final table, from the Embercrest's Sunday $150 to the Grand Circuit and the Championship in Las Vegas.";
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
		BoardScrollGoal -= UI.Ptr.Wheel * Pitch * 1.5f;
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
		NetAvatar(*C, Rr.X + 136.0f, Rr.Y + Rr.H / 2.0f, 14.0f, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, Hero);
		PlayerName(Rw.Player, Rr.X + 160.0f, Ty, 15.0f, 360.0f, true);
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
		const Rect R{24.0f + Nf(I) * (TileW + 12.0f), 296.0f, TileW, 104.0f};
		Panel(R, 12.0f);
		UI.Text(Tiles[I].first, R.X + 18.0f, R.Y + 52.0f, Ts(I >= 4 ? 24.0f : 32.0f, 900, I == 3 && You.Wins > 0 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 30.0f));
		NetSpaced(*C, Tiles[I].second, R.X + 18.0f, R.Y + 82.0f, 9.5f, 800, pal::Muted, 1.2f);
	}

	// Standings.
	Section("LEADERBOARD STANDINGS", 24.0f, 440.0f, pal::Accent);
	const net::Board Boards[5] = {net::Board::NightShift, net::Board::Season, net::Board::Earnings, net::Board::Wins, net::Board::Series};
	const float Sw = (1052.0f - 4.0f * 12.0f) / 5.0f;
	int NightRank = 0;
	for (int I = 0; I < 5; ++I)
	{
		net::BoardRow Row;
		Net.Leaderboard(Boards[I], World, You, 0, &Row);
		NightRank = Boards[I] == net::Board::NightShift ? Row.Rank : NightRank;
		const Rect R{24.0f + Nf(I) * (Sw + 12.0f), 456.0f, Sw, 86.0f};
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
	Section("RECENT TOURNAMENTS", 24.0f, 578.0f, pal::Accent);
	{
		// RiverLine Stats: the player's own dashboard.
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 13.0f;
		O.Enabled = !CardOpen();
		if (UI.Button("careerstats", {1076.0f - 168.0f, 552.0f, 168.0f, 34.0f}, "Your stats  \xE2\x80\xBA", O))
		{
			ShowPlayer(HeroCard, Now);
		}
	}
	if (S.History.empty())
	{
		UI.Text("No tournaments yet. The Night Owl Turbo is running right now.", 24.0f, 620.0f, Ts(16.0f, 500, pal::Muted));
	}
	for (size_t I = 0; I < S.History.size() && I < 6; ++I)
	{
		const HistoryEntry& H = S.History[I];
		const Rect R{24.0f, 594.0f + Nf(I) * 58.0f, 1052.0f, 52.0f};
		UI.RRect(R, 10.0f, Hex(0x101a2b), Rgba(255, 255, 255, 0.03f));
		const bool Cashed = H.Prize > 0;
		C->FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, H.Place == 1 ? pal::Gold : Cashed ? pal::Green : pal::Dim);
		UI.Text(H.Name, R.X + 22.0f, R.Y + 32.0f, Ts(16.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, 420.0f));
		UI.Text(Ordinal(H.Place) + " of " + Grouped(H.Entrants), R.X + 500.0f, R.Y + 32.0f, Ts(15.0f, 700, H.Place <= 9 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, true));
		UI.Text(Cashed ? "+" + Money(H.Prize) : std::string("\xE2\x80\x94"), R.X + 760.0f, R.Y + 32.0f, Ts(15.0f, 800, Cashed ? pal::Green : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		const float Acc = Nf(Clamp01(H.AccuracyPct / 100.0));
		UI.RRect({R.X + 820.0f, R.Y + 24.0f, 120.0f, 6.0f}, 3.0f, Rgba(255, 255, 255, 0.07f));
		UI.RRect({R.X + 820.0f, R.Y + 24.0f, std::max(6.0f, 120.0f * Acc * In), 6.0f}, 3.0f, Acc > 0.85f ? pal::Green : Acc > 0.7f ? pal::Gold : pal::Orange);
		UI.Text(Fixed(H.AccuracyPct, 0) + "%", R.X + R.W - 18.0f, R.Y + 32.0f, Ts(14.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
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
