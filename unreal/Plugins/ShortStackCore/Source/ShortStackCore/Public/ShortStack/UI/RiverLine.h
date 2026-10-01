#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Ui.h"

namespace ss
{
namespace ui
{
/**
 * RiverLine: the fictional poker site running on the laptop. Draws the boot
 * screen, the network lobby (schedule, series, leaderboards, news, career),
 * the table and results into a canvas each frame and turns pointer and key
 * input into session actions. The table and results are a port of
 * web/src/client/riverline.ts; the lobby pages live in RiverLineNet.cpp.
 */
class RiverLine
{
public:
	static constexpr float Width = 1600.0f;
	static constexpr float Height = 1000.0f;

	/** The lobby's pages (top navigation). */
	enum class Page : int
	{
		Lobby,
		Series,
		Leaderboards,
		News,
		Career,
	};
	/** Schedule filters (the chips above the list). */
	enum class Filter : int
	{
		All,
		Playable,
		Micro,
		Low,
		MidHigh,
		Bounty,
		Series,
		Satellites,
		Freerolls,
		Running,
	};

	explicit RiverLine(Session& InSession) : S(InSession) {}

	Ui UI;
	/** Set when the player clicks "Lean back"; the host clears it after handling. */
	bool LeanBackRequested = false;

	/** Draws one frame (and handles the pointer input gathered since the last one). */
	SHORTSTACKCORE_API void Draw(Canvas& C, double Now);
	/** Keyboard shortcuts: "f", "c", "x", "r", "b", "a", "ArrowUp", "ArrowDown" (at the table; in the lobby the arrows move through the schedule). */
	SHORTSTACKCORE_API void Key(const std::string& Key);

	SHORTSTACKCORE_API void OpenPage(Page P, double Now);
	Page CurrentPage() const { return PageShown; }
	SHORTSTACKCORE_API void SetFilter(Filter F, double Now);
	/** Selects a scheduled event by instance id ("mm-26@1530") and scrolls it into view. */
	SHORTSTACKCORE_API void SelectEvent(const std::string& InstanceId);
	const std::string& SelectedEvent() const { return EventId; }
	/** The instance ids in the schedule list as last drawn (top to bottom). */
	const std::vector<std::string>& ListedEvents() const { return Listed; }
	/** Leaderboard shown on the Leaderboards page. */
	SHORTSTACKCORE_API void ShowBoard(net::Board B, double Now);
	/** Series shown on the Series page (id; empty = the one running now). */
	SHORTSTACKCORE_API void ShowSeries(const std::string& Id, double Now);

private:
	void Logo(float X, float Y, float Scale);
	void TopBar(double Now);
	void DrawCursor();
	void Boot(double Now);
	// Network lobby (RiverLineNet.cpp).
	struct ListRow
	{
		net::EventInstance E;
		net::LiveState L;
		bool Joinable = false;
		std::string Lock;
	};
	struct FeatureSlide
	{
		int Art = 0; // 0 chip stacks, 1 trophy, 2 rings, 3 podium, 4 crown
		bool HasEvent = false;
		net::EventInstance E;
		std::string SeriesId;
		uint32_t Col = 0x27d3c3;
		uint32_t Col2 = 0x3b82f6;
		std::string Kicker;
		std::string Title;
		std::string Sub;
		std::string Big;
		std::string BigLabel;
		std::string ClockLabel;
		double ClockAt = 0.0; // world minutes the countdown runs to
		std::string Cta;
		int Action = 0; // 0 register, 1 series page, 2 details, 3 leaderboard
	};
	void NetFrame(double Now);
	void NavTabs(double Now);
	void LobbyPages(double Now);
	void NetLobby(double Now);
	const std::vector<FeatureSlide>& FeatureSlides();
	const std::vector<net::NewsItem>& NewsFeed();
	void DrawSlide(const FeatureSlide& F, const Rect& R, double Now, float Alpha, float Offset, bool Interactive);
	void Featured(const Rect& R, double Now);
	void FilterChips(float X, float Y, float W, double Now, int Count);
	std::vector<ListRow> ScheduleRows() const;
	void Schedule(const Rect& R, const std::vector<ListRow>& Rows, double Now);
	void ScheduleRow(const ListRow& Row, const Rect& R, double Now, int Index);
	void EventPanel(const Rect& R, double Now);
	void RegisterBlock(const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now);
	void Ticker(const Rect& R, double Now);
	void SeriesPage(double Now);
	void BoardsPage(double Now);
	void BoardTable(const Rect& R, const std::vector<net::BoardRow>& Rows, double Now);
	void NewsPage(double Now);
	void CareerPage(double Now);
	std::string BoardValue(net::Board B, double V) const;
	void PlayerName(int Index, float X, float Y, float Size, float MaxW, bool Badges);
	void Panel(const Rect& R, float Radius = 14.0f);
	float Section(const std::string& Title, float X, float Y, const Color& Col);
	float Wrap(const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineHeight);
	std::vector<std::string> WrapLines(const std::string& Text, float MaxW, float Size);
	int HeroSeatNo() const;
	Vec2 SlotPos(int Seat) const;
	Vec2 BetPos(int Seat) const;
	void Table(double Now);
	std::vector<Card> WinningCards() const;
	void DrawSeat(const SeatVis& Seat, double Now);
	Vec2 FlightPoint(const FlightEnd& End) const;
	void DrawFlight(const Flight& F, double Now);
	void GradeBadges(double Now);
	void Controls(double Now);
	double PotFrac(const HeroPrompt& P, double F) const;
	double PotRaise(const HeroPrompt& P) const;
	void SidePanel(double Now);
	void Overlays(double Now);
	void ResultsScreen(double Now);

	float GradeLabel(Grade G, const std::string& Rest, float X, float Y, const TextStyle& Style, bool Draw = true);
	float ArrowText(const std::string& Left, const std::string& Right, float X, float Y, const TextStyle& Style);
	void Ghost(float CX, float CY, float Size);

	Session& S;
	Canvas* C = nullptr;

	// Lobby state.
	Page PageShown = Page::Lobby;
	double PageAt = -100.0;
	Filter FilterShown = Filter::All;
	double FilterAt = -100.0;
	std::string EventId;
	double EventAt = -100.0;
	int DetailTab = 0;
	std::vector<std::string> Listed;
	float ListScroll = 0.0f;
	float ListScrollGoal = 0.0f;
	bool ScrollToSelected = false;
	int SlideShown = 0;
	int SlidePrev = -1;
	double SlideAt = 0.0;
	net::Board BoardShown = net::Board::NightShift;
	double BoardAt = -100.0;
	float BoardScroll = 0.0f;
	float BoardScrollGoal = 0.0f;
	std::string SeriesId;
	int SeriesDay = -9999;
	double SeriesAt = -100.0;
	float NavX = -1.0f;
	float NavW = 0.0f;
	double LastFrame = -1.0;
	double Dt = 0.0;
	double NewsSeenAt = 0.0;
	std::vector<FeatureSlide> Slides;
	long long SlidesKey = -1;
	std::vector<net::NewsItem> News;
	long long NewsKey = -1;
	// This frame.
	double World = 0.0;
	net::HeroStats You;
};
} // namespace ui
} // namespace ss
