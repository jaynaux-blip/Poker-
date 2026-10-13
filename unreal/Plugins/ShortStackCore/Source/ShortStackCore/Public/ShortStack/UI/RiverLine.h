#pragma once

#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Ui.h"

#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlnet_detail
{
struct LiveCard;
}

/**
 * RiverLine: the fictional poker site running on the laptop. Draws the boot
 * screen, the network lobby (schedule, series, leaderboards, news, career, and
 * the week at the Embercrest's card room), the table and results into a canvas
 * each frame and turns pointer and key
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
		Embercrest, // the city's card room: its events this week (live::Occurrences), each on an honest card
		Backing,    // stakes, pieces, swaps and packages (next_segment_spec §2.12; drawn by RiverLineBacking.cpp)
		// Update 2 (G3, ui-laptop's): a rival site's money and offers (RiverLineSites.cpp).
		Promotions,
		Cashier,
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
		PennyDrop, // the Lucky Penny's delivery app: order food, eat and drink at home
		Mail,      // the companies' letters: sponsors, Lantern Row, Finch School (next_segment_spec §3.9; RiverLineMail.cpp)
		Video,     // update 2 (G2): Rerun, the video site (RiverLineVideo.cpp, ui-video's)
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
	/** Drops the per-laptop state the apps keep beside the class (Mail, Backing, tips, badge toasts). */
	SHORTSTACKCORE_API ~RiverLine();

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
	/**
	 * The day the Embercrest page shows (a world day, today to six days out; anything else goes back to the next day
	 * with an event the player can make) and the event picked on it (an occurrence id, "" for none).
	 */
	SHORTSTACKCORE_API void ShowLiveDay(int Day, const std::string& OccurrenceId, double Now);
	int LiveDayShown() const { return LiveDay; }
	/** The Embercrest event whose Register button is waiting for its confirm ("" for none). */
	const std::string& LiveConfirming() const { return LiveConfirm; }
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
	/** Penny Drop: the shelf showing, and the cart (for scripts and tests). */
	void ShowDropShelf(int Shelf) { DropShelf = Shelf; }
	const store::Basket& DropBasket() const { return DropCart; }
	/** Penny Drop's bag at home: its cells in order as last drawn (one used up keeps its cell while the pointer is over the bag), and the page. */
	const std::vector<std::string>& DropBagCells() const { return DropBag; }
	int DropBagPageShown() const { return DropBagPage; }
	/** The LED kit's controls (Prism): colours, power, sync with the stream. Opens over GearDrop or Kast. */
	void ShowRoomLights(bool On, double Now = 0.0) { LightsShown = On; LightsAt = Now; }
	bool RoomLightsShown() const { return LightsShown; }
	void ShowKastPage(KastPage P) { KastShown = P; }
	/** A player card over the RiverLine pages (an index into the network's players; -1 closes it). */
	SHORTSTACKCORE_API void ShowPlayer(int Index, double Now);
	int PlayerShown() const { return CardShown; }
	/** ShowPlayer(HeroCard) opens the player's own stats (from the Career page). */
	static constexpr int HeroCard = -2;
	bool CardOpen() const { return CardShown != -1; }
	/** The open card's view (0: overview, 1: journey, 2: trophies, 3: stats). */
	void ShowCardTab(int Tab) { CardTab = Tab; }
	/** The stats page's big graph: 0 profit, 1 ABI. */
	void ShowStatsGraph(int Graph) { StatsGraph = Graph; }
	/** The event panel's tab: 0 overview, 1 payouts, 2 players (or the final table). */
	void ShowEventTab(int Tab) { DetailTab = Tab; }
	KastPage CurrentKastPage() const { return KastShown; }
	/** Update 2 (G3): the poker site the lobby shows (RiverLine until the rivals open). */
	net::SiteId SiteShown = net::SiteId::RiverLine;

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
	// The Embercrest: its event cards (RiverLineApps.cpp: the Burner's rows) and its page (RiverLineNet.cpp).
	/** Who the world has down for one of the room's events, as the player knows them. */
	struct LivePlan
	{
		int Field = 0; // the world's planned field (0: not planned yet; the world plans a day ahead)
		std::vector<std::pair<std::string, std::string>> Known; // the people in it the player knows, and how
	};
	const LivePlan& PlanOf(const live::Occurrence& O);
	/** The card for O (rlnet_detail::LiveCardOf) with the world's plan for it. */
	rlnet_detail::LiveCard CardFor(const live::Occurrence& O);
	/** One of the room's events in Dee's thread on the Burner: the card's facts in four lines, its warning, its button. */
	void LiveRow(const live::Occurrence& O, const Rect& R, const Color& Tint, const Color& Ink, const Color& Faint, double Now);
	/** Session::GoToLive: the registration (or the way back to an entry) the Burner and the page share; a refusal shows as a toast. */
	void GoToLive(const std::string& OccurrenceId, double Now);
	void EmbercrestPage(double Now);
	void LiveEventCard(const live::Occurrence& O, const Rect& R, double Now, int Index);
	void LiveWeek(const Rect& R, double Now);
	void LiveRoomRules(const Rect& R);
	std::map<std::string, LivePlan> LivePlans;
	long long LivePlansKey = -1;
	int LiveDay = -1;
	double LiveDayAt = -100.0;
	std::string LivePick;
	std::string LiveConfirm;
	void SeriesPage(double Now);
	void BoardsPage(double Now);
	void BoardTable(const Rect& R, const std::vector<net::BoardRow>& Rows, double Now);
	void NewsPage(double Now);
	void CareerPage(double Now);
	std::string BoardValue(net::Board B, double V) const;
	/** Frame: the name's badge frame (badge::Frame: none, champion, gold; next_segment_spec §5.4), drawn by ui-laptop. */
	void PlayerName(int Index, float X, float Y, float Size, float MaxW, bool Badges, uint8_t Frame = 0);
	void PlayerName(const net::Placing& P, float X, float Y, float Size, float MaxW, bool Badges, uint8_t Frame = 0);
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
	/** The whole pot, as the label reads it: what's in the middle plus the bets (and blinds) still in front of the seats. */
	Chips PotTotal() const;
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
	void CardTrophies(const world::Profile& P, float X, float Y, float W, float H, double Now);
	// RiverLine Stats (RiverLineStats.cpp): the dashboard on a card, and the player's own.
	void StatsKpis(const world::Tracker& T, const Rect& R, double Now);
	void StatsBoard(const world::Tracker& T, const std::vector<world::Award>& Awards, const Rect& R, double Now);
	void StatsBrand(float X, float Y);
	void StatsAbi(const world::Tracker& T, const Rect& Pg, const Rect& Plot, double Now);
	int StatsGraph = 0; // 0 profit, 1 ABI
	void HeroStatsCard(double Now);
	int CardShown = -1;
	double CardAt = 0.0;
	int CardTab = 0; // 0: overview, 1: journey, 2: trophies, 3: stats
	double CardTabAt = 0.0;
	// Kast (RiverLineKast.cpp).
	void KastApp(double Now);
	// Penny Drop (RiverLineDrop.cpp).
	void PennyDropApp(double Now);
	void DropCartList(const Rect& R, double Now);
	void DropTracker(const Rect& R, double Now);
	/** The bag at home, in the At home card (R); returns the id of the item under the pointer ("" for none). */
	std::string DropBagGrid(const Rect& R, double Now);
	void DropSay(const std::string& Words, bool Bad, bool Home, double Now);
	store::Basket DropCart;
	store::Basket DropLast; // the last order placed (this session): "Same as last time" refills the cart with it
	int DropShelf = 0;
	std::string DropNote;
	bool DropNoteBad = false;
	bool DropNoteHome = false; // about the bag or the tap (the At home card), not an order
	double DropNoteAt = -100.0;
	int DropCartScroll = 0;        // the first cart line showing
	std::string DropCartFlash;     // the line just added or taken out (it lights up)
	double DropCartFlashAt = -100.0;
	/**
	 * The bag's cells (item ids), in a stable order: something used up keeps its cell, empty, while the pointer is over the
	 * bag, so the next click lands where it was aimed; the bag closes up once the pointer leaves it.
	 */
	std::vector<std::string> DropBag;
	std::map<std::string, float> DropBagSlide; // where each cell is drawn (eases to its slot as the bag closes up)
	int DropBagPage = 0;
	double DropBagPageAt = -100.0;
	std::map<std::string, int> DropBagSeen;      // the bag as last drawn, to see what arrived
	std::map<std::string, double> DropBagNewAt; // when something arrived in the bag (it wears a NEW tag for a while)
	bool DropBagKnown = false;
	size_t DropOrdersSeen = 0;
	double DropArrivedAt = -100.0;
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
	// The next segment (next_segment_spec §7.3; declared in Phase A). The Backing page (RiverLineBacking.cpp, core-stake),
	// the Mail app (RiverLineMail.cpp, core-sponsor), the HONORS panel and a badge's hover line (ui-laptop, with core-badges).
	void BackingPage(double Now);
	void MailApp(double Now);
	void HonorsPanel(const Rect& R, double Now);
	void BadgeTip(const badge::Badge& B, float X, float Y, double Now);
	bool HonorsShown = false;
	// Multi-tabling (RiverLineTables.cpp).
	void TableStrip(float X, double Now);
	void Tiles(double Now);
	void Tile(int Index, const Rect& R, bool Front, double Now);
	void FinishedToasts(double Now);
	int TileClicked = -1;
	// Update 2 (declared in Phase A; side state beside the class, as Mail's). Rerun (RiverLineVideo.cpp, ui-video);
	// autoplay's panel, the watch strip and the skip card (RiverLineAuto.cpp, ui-laptop); the rival sites' Promotions and
	// Cashier pages, the poker button and Career's Rewards card (RiverLineSites.cpp, ui-laptop).
	void VideoApp(double Now);
	void AutoPanel(const Rect& R, double Now);
	void AutoStrip(const Rect& R, double Now);
	void SkipCard(const Rect& R, double Now);
	void SitePromotions(double Now);
	void SiteCashier(double Now);
	void PokerButton(const Rect& R, double Now);
	void RewardsCard(const Rect& R, double Now);

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
