#pragma once

#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Ui.h"

#include <functional>

namespace ss
{
namespace ui
{
/** Everything the player can change under Settings. The host applies it (in Unreal: scalability, ray tracing, the mix). */
struct GameSettings
{
	// Graphics
	int Quality = 3;             // 0 Low, 1 Medium, 2 High, 3 Epic, 4 Cinematic
	bool RayTracing = true;      // hardware ray-traced global illumination and reflections
	int ResolutionScale = 100;   // percent of the output resolution (over 100 supersamples); with dynamic resolution, the most it renders
	int DynamicTarget = 60;      // frames per second the render scale moves to hold (0: a fixed resolution scale)
	int FrameRateLimit = 0;      // frames per second, 0 = unlimited
	bool VSync = false;
	bool MotionBlur = false;
	int FilmGrain = 1;           // 0 Off, 1 Subtle, 2 Strong
	bool ChromaticAberration = true;
	// Display
	int WindowMode = 0;          // 0 Fullscreen, 1 Borderless, 2 Windowed
	std::string Resolution;      // "2560 x 1440"; empty = the display's native resolution
	int Brightness = 50;         // 0..100, 50 = neutral exposure
	int FieldOfView = 50;        // vertical degrees
	// Audio
	int MasterVolume = 80;
	int EffectsVolume = 100;
	int AmbienceVolume = 100;
	bool BackgroundAudio = true; // keep playing with another window in front
	// Controls
	int LookSensitivity = 100;   // percent
	bool InvertLook = false;
	bool ShowHints = true;
	// Gameplay
	int TablePace = 0;           // a live table: 0 Live (the room's own time), 1 Brisk, 2 Fast; never plays a hand for you

	/** "key=value" lines, safe to store in any save system. Unknown keys are ignored when parsing. */
	SHORTSTACKCORE_API std::string Serialize() const;
	static SHORTSTACKCORE_API bool Parse(const std::string& Text, GameSettings& Out);
};

/** What the menus show about the player's career and the night so far. */
struct FrontEndInfo
{
	bool HasSave = false;
	std::string HeroName = "grinder_3c";
	/** The person behind it (Created false for a career from before the creator). */
	hero::Character Person;
	Chips BankrollCents = 237;
	int Tournaments = 0;
	std::string BestFinish; // "12th of 180"
	std::string LastResult; // "Night Owl Turbo \xC2\xB7 45th of 1,000"
	/** Pause menu: label and value pairs about the session in progress. */
	std::vector<std::pair<std::string, std::string>> Status;
	bool InTournament = false;
	/** The session's clock on the network's calendar (world minutes): 2:07 AM, Tuesday, October 6, for a new career. */
	double World = 1440.0 + 127.0;
	/** The rent as it stands (the CONTINUE card says what's owed and by when), and the storage unit's renewal (0: none). */
	life::Rent RentStage = life::Rent::Due;
	Chips RentDueCents = 122500;
	double RentDeadline = 5.0 * 1440.0;
	double StorageDue = 0.0;
	/** Display resolutions the host supports, as "2560 x 1440", largest first. */
	std::vector<std::string> Resolutions;
	std::string Version = "v0.2";
};

/** Fills FrontEndInfo from the session (career from the save, status from the tournament in progress). */
SHORTSTACKCORE_API FrontEndInfo DescribeSession(const Session& S, bool HasSave);

/** Where play is going on when the game pauses: the pause menu's heading and its quit copy follow it. */
enum class Venue : int
{
	Home,     // the rented room, at the desk (Dee's place while the locks are changed)
	Street,   // out on Fifth: the sidewalk and the Lucky Penny
	BackRoom, // Dee's game behind the laundromat
	CardRoom, // the Embercrest's tournament floor
};

/** How the front end reaches the game. */
class FrontEndHooks
{
public:
	virtual ~FrontEndHooks() = default;
	virtual void UiSound(SoundId /*Id*/, double /*Volume*/) {}
	virtual void Continue() {}
	virtual void NewGame(const std::string& /*ScreenName*/) {}
	/** A new career for the character the creator made. Hosts from before the creator get NewGame. */
	virtual void NewCareer(const std::string& ScreenName, const hero::Character& /*Who*/) { NewGame(ScreenName); }
	virtual void Resume() {}
	virtual void QuitToMenu() {}
	virtual void QuitGame() {}
	virtual void SettingsChanged(const GameSettings& /*Settings*/) {}
};

/**
 * The title screen, main menu, new game, settings, credits and the in-game
 * pause menu. Drawn full screen over the live 3D scene into a canvas whose
 * height is 1080 logical units and whose width follows the viewport's aspect
 * ratio. Input is the pointer (in logical units) plus key names:
 * "Up", "Down", "Left", "Right", "Enter", "Space", "Escape", "Backspace",
 * "Tab", "Q", "E", "R", "TabPrev", "TabNext", "Reset", "PutBack" (the
 * gamepad's X), "Undo" (Ctrl+Z), and "Any" for any other key. Printable
 * characters arrive through Char, after the key that typed them.
 */
class FrontEnd
{
public:
	enum class Page : int
	{
		Hidden,
		Attract,
		Main,
		NewGame,
		Settings,
		Credits,
		Pause,
	};
	static constexpr float Height = 1080.0f;

	explicit FrontEnd(FrontEndHooks& InHooks) : Hooks(InHooks) {}
	FrontEnd(const FrontEnd&) = delete;
	FrontEnd& operator=(const FrontEnd&) = delete;

	GameSettings Settings;
	FrontEndInfo Info;
	/** Pointer in logical units (Height = 1080). */
	Pointer Ptr;
	/** Show gamepad button glyphs instead of keys (the host sets it from the last device used). */
	bool Gamepad = false;
	/** The screen name typed on the New Game page. */
	std::string ScreenName = "grinder_3c";
	/** The person the character creator is making (New Game). */
	hero::Character Draft;
	/**
	 * Mixed into the creator's dice, so the faces it offers differ from one launch to the next (the host sets it once,
	 * from the wall clock, when it makes the menu). The moment the creator opens is in the mix too.
	 */
	std::string CreatorSalt;
	/**
	 * Where play is going on: the host sets it as its level starts (Home by default). The pause menu's heading names it
	 * ("FIFTH STREET \xC2\xB7 TUE \xC2\xB7 2:16 AM", or Label when given) and the quit copy follows it.
	 */
	SHORTSTACKCORE_API void SetVenue(Venue Place, const std::string& Label = std::string());
	Venue CurrentVenue() const { return Where; }
	/** The creator's steps, in order. */
	enum class CreatorStage : int
	{
		Identity,
		Background,
		Look,
		Review,
	};
	CreatorStage CreatorStep() const { return Stage; }
	int CreatorField() const { return Field; }
	int CreatorTab() const { return LookTab; }

	/** Attract at boot, Main after quitting to the menu, Pause during play. */
	SHORTSTACKCORE_API void Open(Page Target, double Now);
	/** Fades the menu out and hands input back to the game. */
	SHORTSTACKCORE_API void Close(double Now);
	Page Current() const { return Cur; }
	bool IsOpen() const { return Cur != Page::Hidden; }
	/** Opened from play: the game is paused behind it. */
	bool IsPaused() const { return Cur != Page::Hidden && InGame; }
	/** The title flow frames the room with the establishing shot instead of the seat. */
	bool WantsEstablishingShot() const { return Cur != Page::Hidden && !InGame; }
	/** 0..1: how strongly the host should blur the scene behind the menu (the pause menu; softly, behind the creator). */
	SHORTSTACKCORE_API float Backdrop(double Now) const;

	SHORTSTACKCORE_API void Key(const std::string& Name, double Now);
	SHORTSTACKCORE_API void Char(uint32_t Codepoint, double Now);
	/** Draws one frame and handles the pointer input gathered since the last one. */
	SHORTSTACKCORE_API void Draw(Canvas& Cv, double Now);

private:
	struct MenuItem
	{
		std::string Label;
		bool Enabled = true;
	};
	struct SettingRow
	{
		std::string Label;
		std::string Help;
		int Impact = -1; // GPU cost 0..3, or -1 to hide
		bool Slider = false;
		std::vector<std::string> Options;
		int Min = 0;
		int Max = 100;
		int Step = 5;
		std::string Suffix;
		std::function<int()> Get;
		std::function<void(int)> Set;
	};
	enum class Modal : int
	{
		None,
		QuitGame,
		QuitToMenu,
		Overwrite, // BEGIN with a career saved: start over?
	};

	void HandleKey(const std::string& Name, double Now);
	void Confirmed(Modal Which, double Now);
	void Go(Page Target, double Now);
	std::vector<MenuItem> Items() const;
	void Activate(int Index, double Now);
	void MoveSelection(int Delta);
	std::vector<SettingRow> Rows(int ForTab);
	void ChangeSetting(int RowIndex, int Delta, bool Wrap);
	void BeginNewGame(double Now);
	void StartCareer(double Now);
	bool NameValid() const;

	// The character creator (FrontEndCreator.cpp): New Game's four steps around a live portrait.
	void CreatorOpen(double Now);
	void CreatorGo(CreatorStage Target, double Now);
	void CreatorNext(double Now);
	void CreatorBack(double Now);
	void CreatorKey(const std::string& Name, double Now);
	void CreatorChar(uint32_t Codepoint, double Now);
	void CreatorRandomize(bool LookOnly, double Now);
	void PushUndo();
	bool CreatorCanUndo() const;
	void CreatorUndo(double Now);
	void CreatorChange(int Delta, bool Wrap);
	void CreatorSuggest(int Delta);
	void CreatorRefuse(const std::string& Why, double Now);
	void CreatorTouched() { DraftTouched = true; }
	void TidyNames();
	std::string CreatorProblem() const;
	std::string* FocusedText();
	int CreatorRows() const;
	void CreatorHeader(double Now);
	void CreatorPortrait(const Rect& R, double Now);
	void IdentityStep(double Now);
	void BackgroundStep(double Now);
	void LookStep(double Now);
	void ReviewStep(double Now);
	void PlayersCard(const Rect& R, double Now);
	bool TextBox(const Rect& R, const std::string& Label, const std::string& Value, int Index, double Now);
	void Sound(SoundId Id, double Volume) { Hooks.UiSound(Id, Volume); }

	// Drawing (Draw sets C, ViewW, Fade and PageTime for the page being drawn).
	void DrawPage(Page P, double Now);
	void Scrim(Page P);
	void Logo(float X, float Y, float Size, bool Centered, double Now);
	void MenuList(const std::vector<MenuItem>& List, float X, float Y, double Now);
	void ContextCard(float X, float Y, float CardW, double Now);
	float ContextBody(float X, float Y, float CardW);
	std::string VenueLabel() const;
	/** The pause menu's quit cards and dialog: what leaving looks like from here, and what's kept. */
	std::string LeaveTitle(bool ToMenu) const;
	std::string SavedLine() const;
	/** The control hints along the bottom; returns where they end. HintsWidth measures without drawing. */
	float Hints(const std::vector<std::pair<std::string, std::string>>& Pairs);
	float HintsWidth(const std::vector<std::pair<std::string, std::string>>& Pairs) const;
	void Footer();
	void AttractPage(double Now);
	void MainPage(double Now);
	void NewGamePage(double Now);
	void SettingsPage(double Now);
	void CreditsPage(double Now);
	void PausePage(double Now);
	void ModalBox(double Now);
	bool Button(const Rect& R, const std::string& Label, bool Primary, bool Focused);
	bool Released(const Rect& R) const;
	Color F(Color Col, float Alpha = 1.0f) const
	{
		Col.A *= Alpha * Fade;
		return Col;
	}

	FrontEndHooks& Hooks;
	Page Cur = Page::Hidden;
	Page Prev = Page::Hidden;
	double PageAt = -10.0;
	bool InGame = false;
	Page SettingsReturn = Page::Main;
	int Sel = 0;
	int SelTab = 0;
	int SelRow = 0;
	double SelAt = 0.0;
	Modal Confirm = Modal::None;
	double ConfirmAt = 0.0;
	int ConfirmSel = 1;
	std::vector<float> Hot;
	double LastNow = 0.0;
	float LastPtrX = -1.0f;
	float LastPtrY = -1.0f;
	float PressX = -1.0f;
	float PressY = -1.0f;
	bool SliderDrag = false;
	double NameErrorAt = -10.0;
	CreatorStage Stage = CreatorStage::Identity;
	int Field = 0;
	int LookTab = 0;
	double StageAt = 0.0;
	std::string Refusal;
	double RefusalAt = -10.0;
	int Creations = 0;
	int Rolls = 0;
	/** The player changed the draft: backing out to the menu and in again keeps it (a fresh face otherwise). */
	bool DraftTouched = false;
	/** Drafts before each randomize (and a typed name a suggestion replaced), newest last: undo. */
	std::vector<hero::Character> Undo;
	/**
	 * The draft as the last roll, suggestion or undo left it. Undo only takes a roll back while the draft still matches
	 * it: once the player has changed something by hand, one key mustn't wipe that work along with the roll.
	 */
	hero::Character Rolled;
	double UndoAt = -10.0;
	/** The portrait's rim light eases from the last background's color to the new one. */
	Color RimFrom = Hex(0x27d3c3);
	Color RimTo = Hex(0x27d3c3);
	double RimAt = -10.0;
	/** The key just handled opened a page: the character it types (a space on NEW GAME) mustn't land there. */
	bool SwallowChar = false;
	Venue Where = Venue::Home;
	std::string WhereLabel;
	double CreditsOffset = 0.0;
	double CreditsManualAt = -10.0;

	bool PtrMoved = false;    // the pointer moved this frame (hover changes the selection only then)
	bool Interactive = false; // false while a page is only fading out
	bool CardPaint = true;    // ContextBody draws (true) or only measures (false)
	double FrameDt = 0.0;

	Canvas* C = nullptr;
	float ViewW = 1920.0f;
	float Fade = 1.0f;
	double PageTime = 0.0; // seconds since the page being drawn opened
};
} // namespace ui
} // namespace ss
