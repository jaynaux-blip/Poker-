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
 * input into session actions. With several tables open (multi-tabling) it shows
 * them as tabs in the top bar, one in front, or tiled side by side. The table and results are a port of
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

	/** The laptop's other apps (the taskbar along the bottom of the screen). */
	enum class App : int
	{
		RiverLine,
		ShiftLink, // gig shifts
		Burner,    // Marcus and Sam
		Bank,      // balance, rent, history
		GearDrop,  // the store: screens, the rig, stream gear, the apartment
		Kast,      // streaming: the studio, the channel, the directory
	};
	/** Kast's pages. */
	enum class KastPage : int
	{
		Studio,
		Community,
		Channel,
		Browse,
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
	SHORTSTACKCORE_API void OpenApp(App A, double Now);
	App CurrentApp() const { return AppShown; }
	/** Burner contact shown: 0 Marcus, 1 Sam. */
	void ShowContact(int Contact) { BurnerContact = Contact; }
	/** The sleep menu over the taskbar. */
	void ShowSleepMenu(bool Open) { SleepOpen = Open; }
	/** GearDrop's category (-1 all, else gear::Category). */
	void ShowStoreCategory(int Category) { StoreCat = Category; StoreScroll = StoreScrollGoal = 0.0f; }
	/** Opens the order card for an item (the confirm step). */
	void ShowOrder(const std::string& ItemId) { OrderId = ItemId; }
	const std::string& OrderShown() const { return OrderId; }
	/** The LED kit's controls (Prism): colours, power, sync with the stream. Opens over GearDrop or Kast. */
	void ShowRoomLights(bool On, double Now = 0.0) { LightsShown = On; LightsAt = Now; }
	bool RoomLightsShown() const { return LightsShown; }
	void ShowKastPage(KastPage P) { KastShown = P; }
	/** A player card over the RiverLine pages (an index into the network's players; -1 closes it). */
	SHORTSTACKCORE_API void ShowPlayer(int Index, double Now);
	int PlayerShown() const { return CardShown; }
	/** The open card's view (0: overview, 1: journey). */
	void ShowCardTab(int Tab) { CardTab = Tab; }
	/** The event panel's tab: 0 overview, 1 payouts, 2 players (or the final table). */
	void ShowEventTab(int Tab) { DetailTab = Tab; }
	KastPage CurrentKastPage() const { return KastShown; }

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
	// Laptop apps (RiverLineApps.cpp).
	void Taskbar(const Rect& R, double Now);
	void AppIcon(App A, float X, float Y, float Size);
	bool AppButton(const std::string& Id, const Rect& R, const std::string& Label, const Color& Fill, const Color& Ink, bool Enabled, const std::string& Sub = std::string());
	void ShiftLinkApp(double Now);
	void BurnerApp(double Now);
	void BankApp(double Now);
	void SleepMenu(double Now);
	void SkipOverlay(double Now);
	void OutcomeCard(double Now);
	void TryActivity(const std::string& Id, double Now);
	void SeriesPage(double Now);
	void BoardsPage(double Now);
	void BoardTable(const Rect& R, const std::vector<net::BoardRow>& Rows, double Now);
	void NewsPage(double Now);
	void CareerPage(double Now);
	std::string BoardValue(net::Board B, double V) const;
	void PlayerName(int Index, float X, float Y, float Size, float MaxW, bool Badges);
	void PlayerName(const net::Placing& P, float X, float Y, float Size, float MaxW, bool Badges);
	std::string PlacingName(const net::Placing& P) const;
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
	// GearDrop (RiverLineStore.cpp).
	void GearDropApp(double Now);
	void StoreCard(const gear::Item& I, const Rect& R, double Now, int Index);
	void SetupPanel(const Rect& R, double Now);
	void OrderCard(double Now);
	void RoomLightsCard(double Now);
	void PlayerCard(double Now);
	/** The card's journey: how they arrived, and every step since. */
	void CardJourney(const world::Profile& P, float X, float Y, float W, float H, double Now);
	int CardShown = -1;
	double CardAt = 0.0;
	int CardTab = 0; // 0: overview, 1: journey
	double CardTabAt = 0.0;
	// Kast (RiverLineKast.cpp).
	void KastApp(double Now);
	void KastHeader(double Now);
	void KastStudio(double Now);
	void KastCommunity(double Now);
	void KastChannel(double Now);
	void KastBrowse(double Now);
	void StreamPreview(const Rect& R, double Now);
	void StreamOverlay(const Rect& R, float Scale, double Now);
	void StreamChat(const Rect& R, double Now, bool Interactive);
	void StreamChatLine(const kast::ChatMsg& M, float X, float Y, float W, float Size, bool Interactive, double Now, float& LineHeight, bool Measure);
	void StreamAlert(float Cx, float Y, float Scale, double Now);
	void StreamSummary(double Now);
	void LivePill(float X, float Y, double Now);
	void StreamSide(const Rect& R, double Now);
	float EmoteText(const std::string& Text, float X, float Y, float Size, int Weight, const Color& Col, float MaxW, bool Draw);
	// Multi-tabling (RiverLineTables.cpp).
	void TableStrip(float X, double Now);
	void Tiles(double Now);
	void Tile(int Index, const Rect& R, bool Front, double Now);
	void FinishedToasts(double Now);
	int TileClicked = -1;

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
	App AppShown = App::RiverLine;
	double AppAt = -100.0;
	bool SleepOpen = false;
	int BurnerContact = 0;
	std::string Toast;
	double ToastAt = -100.0;
	// GearDrop.
	int StoreCat = -1;
	float StoreScroll = 0.0f;
	float StoreScrollGoal = 0.0f;
	std::string OrderId;
	bool LightsShown = false;
	double LightsAt = -100.0;
	std::string Delivered;
	double DeliveredAt = -100.0;
	// Kast.
	KastPage KastShown = KastPage::Studio;
	double KastAt = -100.0;
	int ChatTab = 0; // 0 chat, 1 mods, 2 activity
	float ChannelScroll = 0.0f;
	float ChannelScrollGoal = 0.0f;
	bool Previewing = false; // drawing RiverLine into the stream preview (no input, no side effects)
	// This frame.
	double World = 0.0;
	net::HeroStats You;
};
} // namespace ui
} // namespace ss
