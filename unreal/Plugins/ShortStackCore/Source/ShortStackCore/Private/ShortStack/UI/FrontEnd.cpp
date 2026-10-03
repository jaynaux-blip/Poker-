#include "ShortStack/UI/FrontEnd.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include "ShortStack/Game/Format.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ss
{
namespace ui
{

using namespace frontend_detail;

// ------------------------------------------------------------------ settings

std::string GameSettings::Serialize() const
{
	std::string Out = "shortstack.settings.v1\n";
	auto Put = [&Out](const char* Name, int Value) {
		Out += Name;
		Out += '=';
		Out += std::to_string(Value);
		Out += '\n';
	};
	Put("quality", Quality);
	Put("raytracing", RayTracing ? 1 : 0);
	Put("resolutionscale", ResolutionScale);
	Put("fpslimit", FrameRateLimit);
	Put("vsync", VSync ? 1 : 0);
	Put("motionblur", MotionBlur ? 1 : 0);
	Put("filmgrain", FilmGrain);
	Put("chromatic", ChromaticAberration ? 1 : 0);
	Put("windowmode", WindowMode);
	Put("brightness", Brightness);
	Put("fov", FieldOfView);
	Put("master", MasterVolume);
	Put("effects", EffectsVolume);
	Put("ambience", AmbienceVolume);
	Put("bgaudio", BackgroundAudio ? 1 : 0);
	Put("sensitivity", LookSensitivity);
	Put("invert", InvertLook ? 1 : 0);
	Put("hints", ShowHints ? 1 : 0);
	Out += "resolution=" + Resolution + "\n";
	return Out;
}

bool GameSettings::Parse(const std::string& Text, GameSettings& Out)
{
	if (Text.rfind("shortstack.settings.v1", 0) != 0)
	{
		return false;
	}
	GameSettings S;
	size_t Pos = 0;
	while (Pos < Text.size())
	{
		size_t End = Text.find('\n', Pos);
		if (End == std::string::npos)
		{
			End = Text.size();
		}
		const std::string Line = Text.substr(Pos, End - Pos);
		Pos = End + 1;
		const size_t Eq = Line.find('=');
		if (Eq == std::string::npos)
		{
			continue;
		}
		const std::string Name = Line.substr(0, Eq);
		const std::string Value = Line.substr(Eq + 1);
		const int N = std::atoi(Value.c_str());
		if (Name == "quality") S.Quality = std::clamp(N, 0, 4);
		else if (Name == "raytracing") S.RayTracing = N != 0;
		else if (Name == "resolutionscale") S.ResolutionScale = std::clamp(N, 50, 100);
		else if (Name == "fpslimit") S.FrameRateLimit = std::clamp(N, 0, 1000);
		else if (Name == "vsync") S.VSync = N != 0;
		else if (Name == "motionblur") S.MotionBlur = N != 0;
		else if (Name == "filmgrain") S.FilmGrain = std::clamp(N, 0, 2);
		else if (Name == "chromatic") S.ChromaticAberration = N != 0;
		else if (Name == "windowmode") S.WindowMode = std::clamp(N, 0, 2);
		else if (Name == "brightness") S.Brightness = std::clamp(N, 0, 100);
		else if (Name == "fov") S.FieldOfView = std::clamp(N, 40, 70);
		else if (Name == "master") S.MasterVolume = std::clamp(N, 0, 100);
		else if (Name == "effects") S.EffectsVolume = std::clamp(N, 0, 100);
		else if (Name == "ambience") S.AmbienceVolume = std::clamp(N, 0, 100);
		else if (Name == "bgaudio") S.BackgroundAudio = N != 0;
		else if (Name == "sensitivity") S.LookSensitivity = std::clamp(N, 20, 200);
		else if (Name == "invert") S.InvertLook = N != 0;
		else if (Name == "hints") S.ShowHints = N != 0;
		else if (Name == "resolution") S.Resolution = Value;
	}
	Out = S;
	return true;
}

// ------------------------------------------------------------------ session facts

FrontEndInfo DescribeSession(const Session& S, bool HasSave)
{
	FrontEndInfo Out;
	Out.HasSave = HasSave;
	Out.HeroName = S.HeroName;
	Out.Person = S.Person;
	Out.BankrollCents = S.BankrollCents;
	Out.Tournaments = static_cast<int>(S.History.size());
	const HistoryEntry* Best = nullptr;
	for (const HistoryEntry& E : S.History)
	{
		if (E.Place > 0 && (!Best || E.Place < Best->Place))
		{
			Best = &E;
		}
	}
	if (Best)
	{
		Out.BestFinish = Ordinal(Best->Place) + " of " + Grouped(Best->Entrants);
	}
	if (!S.History.empty())
	{
		const HistoryEntry& Last = S.History.back();
		Out.LastResult = Last.Name + " \xC2\xB7 " + Ordinal(Last.Place);
	}
	Out.Status.push_back({"BANKROLL", Money(S.BankrollCents)});
	Out.Status.push_back({"TIME", ClockString(S.ClockMinutes())});
	Out.InTournament = S.T && !S.T->bFinished && S.CurrentScreen == Screen::Table && !S.T->Hero().Busted;
	if (Out.InTournament)
	{
		Out.Status.push_back({"EVENT", S.Event ? S.Event->Name : S.T->Spec.Name});
		Out.Status.push_back({"PLAYERS LEFT", Grouped(S.T->Remaining)});
		Out.Status.push_back({"YOUR RANK", Ordinal(S.T->HeroRank())});
		Out.Status.push_back({"STACK", ChipsText(static_cast<double>(S.T->Hero().Stack))});
	}
	return Out;
}

// ------------------------------------------------------------------ navigation

void FrontEnd::Open(Page Target, double Now)
{
	InGame = Target == Page::Pause;
	Confirm = Modal::None;
	Go(Target, Now);
	if (InGame)
	{
		Sound(SoundId::Click, 0.5);
	}
}

void FrontEnd::Close(double Now)
{
	Prev = Cur;
	Cur = Page::Hidden;
	PageAt = Now;
	Confirm = Modal::None;
	SliderDrag = false;
}

void FrontEnd::Go(Page Target, double Now)
{
	Prev = Cur;
	Cur = Target;
	PageAt = Now;
	SelAt = Now;
	SliderDrag = false;
	Hot.assign(8, 0.0f);
	switch (Target)
	{
	case Page::Main: Sel = Info.HasSave ? 0 : 1; break;
	case Page::Pause: Sel = 0; break;
	case Page::Settings: SelRow = 0; break;
	case Page::Credits:
		CreditsOffset = 0.0;
		CreditsManualAt = -10.0;
		break;
	case Page::NewGame:
		if (ScreenName.empty())
		{
			ScreenName = Info.HeroName;
		}
		NameErrorAt = -10.0;
		CreatorOpen(Now);
		break;
	default: break;
	}
}

float FrontEnd::Backdrop(double Now) const
{
	const float K = Ease((Now - PageAt) / 0.35);
	if (Cur != Page::Hidden)
	{
		return InGame ? (Prev == Page::Hidden ? K : 1.0f) : 0.0f;
	}
	return InGame && Prev != Page::Hidden ? 1.0f - K : 0.0f;
}

std::vector<FrontEnd::MenuItem> FrontEnd::Items() const
{
	if (Cur == Page::Pause || (Cur == Page::Hidden && Prev == Page::Pause))
	{
		return {{"RESUME", true}, {"SETTINGS", true}, {"QUIT TO MAIN MENU", true}, {"QUIT TO DESKTOP", true}};
	}
	return {{"CONTINUE", Info.HasSave}, {"NEW GAME", true}, {"SETTINGS", true}, {"CREDITS", true}, {"QUIT GAME", true}};
}

void FrontEnd::MoveSelection(int Delta)
{
	const std::vector<MenuItem> List = Items();
	const int N = static_cast<int>(List.size());
	for (int Step = 0; Step < N; ++Step)
	{
		Sel = (Sel + Delta + N) % N;
		if (List[static_cast<size_t>(Sel)].Enabled)
		{
			break;
		}
	}
	SelAt = LastNow;
	Sound(SoundId::Click, 0.3);
}

void FrontEnd::Activate(int Index, double Now)
{
	const std::vector<MenuItem> List = Items();
	if (Index < 0 || Index >= static_cast<int>(List.size()) || !List[static_cast<size_t>(Index)].Enabled)
	{
		return;
	}
	if (Cur == Page::Pause)
	{
		switch (Index)
		{
		case 0:
			Sound(SoundId::Chip, 0.5);
			Close(Now);
			Hooks.Resume();
			break;
		case 1:
			Sound(SoundId::Chip, 0.5);
			SettingsReturn = Page::Pause;
			Go(Page::Settings, Now);
			break;
		case 2:
			Sound(SoundId::Chip, 0.5);
			Confirm = Modal::QuitToMenu;
			ConfirmAt = Now;
			ConfirmSel = 1;
			break;
		default:
			Sound(SoundId::Chip, 0.5);
			Confirm = Modal::QuitGame;
			ConfirmAt = Now;
			ConfirmSel = 1;
			break;
		}
		return;
	}
	switch (Index)
	{
	case 0:
		Sound(SoundId::ChipStack, 0.7);
		Close(Now);
		Hooks.Continue();
		break;
	case 1:
		Sound(SoundId::Chip, 0.5);
		ScreenName = Info.HeroName;
		Go(Page::NewGame, Now);
		break;
	case 2:
		Sound(SoundId::Chip, 0.5);
		SettingsReturn = Page::Main;
		Go(Page::Settings, Now);
		break;
	case 3:
		Sound(SoundId::Chip, 0.5);
		Go(Page::Credits, Now);
		break;
	default:
		Sound(SoundId::Chip, 0.5);
		Confirm = Modal::QuitGame;
		ConfirmAt = Now;
		ConfirmSel = 1;
		break;
	}
}

bool FrontEnd::NameValid() const
{
	if (ScreenName.size() < 3 || ScreenName.size() > 16)
	{
		return false;
	}
	for (const char Ch : ScreenName)
	{
		const bool Ok = (Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z') || (Ch >= '0' && Ch <= '9') || Ch == '_' || Ch == '.' || Ch == '-';
		if (!Ok)
		{
			return false;
		}
	}
	return true;
}

void FrontEnd::Key(const std::string& Name, double Now)
{
	if (Cur == Page::Hidden)
	{
		return;
	}
	const bool Up = Name == "Up";
	const bool Down = Name == "Down";
	const bool Left = Name == "Left";
	const bool Right = Name == "Right";
	const bool Enter = Name == "Enter" || (Name == "Space" && Cur != Page::NewGame);
	// "P" also backs out during play (Escape stops Play-In-Editor sessions before the game sees it).
	const bool Back = Name == "Escape" || (Name == "P" && InGame);

	if (Confirm != Modal::None)
	{
		if (Up || Down || Left || Right || Name == "Tab")
		{
			ConfirmSel = 1 - ConfirmSel;
			Sound(SoundId::Click, 0.3);
		}
		else if (Enter)
		{
			const Modal Which = Confirm;
			Confirm = Modal::None;
			if (ConfirmSel == 0)
			{
				Sound(SoundId::Fold, 0.6);
				if (Which == Modal::QuitGame)
				{
					Hooks.QuitGame();
				}
				else
				{
					Hooks.QuitToMenu();
				}
			}
			else
			{
				Sound(SoundId::Check, 0.4);
			}
		}
		else if (Back)
		{
			Confirm = Modal::None;
			Sound(SoundId::Check, 0.4);
		}
		return;
	}

	switch (Cur)
	{
	case Page::Attract:
		Sound(SoundId::ChipStack, 0.6);
		Go(Page::Main, Now);
		break;
	case Page::Main:
	case Page::Pause:
		if (Up || Down)
		{
			MoveSelection(Up ? -1 : 1);
		}
		else if (Enter)
		{
			Activate(Sel, Now);
		}
		else if (Back)
		{
			if (Cur == Page::Pause)
			{
				Activate(0, Now);
			}
			else
			{
				Sound(SoundId::Chip, 0.5);
				Confirm = Modal::QuitGame;
				ConfirmAt = Now;
				ConfirmSel = 1;
			}
		}
		break;
	case Page::NewGame: CreatorKey(Name, Now); break;
	case Page::Settings:
	{
		const int Count = static_cast<int>(Rows(SelTab).size());
		if (Up || Down)
		{
			SelRow = (SelRow + (Up ? -1 : 1) + Count) % Count;
			SelAt = Now;
			Sound(SoundId::Click, 0.3);
		}
		else if (Left || Right)
		{
			ChangeSetting(SelRow, Left ? -1 : 1, false);
		}
		else if (Enter)
		{
			ChangeSetting(SelRow, 1, true);
		}
		else if (Name == "Q" || Name == "TabPrev" || Name == "E" || Name == "Tab" || Name == "TabNext")
		{
			SelTab = (SelTab + (Name == "Q" || Name == "TabPrev" ? 3 : 1)) % 4;
			SelRow = 0;
			SelAt = Now;
			Sound(SoundId::Click, 0.4);
		}
		else if (Name == "R" || Name == "Reset")
		{
			Settings = GameSettings();
			Hooks.SettingsChanged(Settings);
			Sound(SoundId::ChipStack, 0.5);
		}
		else if (Back)
		{
			Sound(SoundId::Check, 0.4);
			const Page Return = SettingsReturn;
			Go(Return, Now);
			Sel = Return == Page::Pause ? 1 : 2;
		}
		break;
	}
	case Page::Credits:
		if (Up || Down)
		{
			CreditsOffset = std::max(0.0, CreditsOffset + (Up ? -90.0 : 90.0));
			CreditsManualAt = Now;
		}
		else if (Back || Enter)
		{
			Sound(SoundId::Check, 0.4);
			Go(Page::Main, Now);
			Sel = 3;
		}
		break;
	default: break;
	}
}

void FrontEnd::Char(uint32_t Codepoint, double Now)
{
	if (Cur == Page::Attract)
	{
		Key("Any", Now);
		return;
	}
	if (Cur == Page::NewGame && Confirm == Modal::None)
	{
		CreatorChar(Codepoint, Now);
	}
}

// ------------------------------------------------------------------ settings rows

std::vector<FrontEnd::SettingRow> FrontEnd::Rows(int ForTab)
{
	GameSettings& S = Settings;
	std::vector<SettingRow> Out;
	auto Choice = [&Out](const std::string& Label, const std::string& Help, int Impact, std::vector<std::string> Options, std::function<int()> Get, std::function<void(int)> Set) {
		SettingRow R;
		R.Label = Label;
		R.Help = Help;
		R.Impact = Impact;
		R.Options = std::move(Options);
		R.Max = static_cast<int>(R.Options.size()) - 1;
		R.Get = std::move(Get);
		R.Set = std::move(Set);
		Out.push_back(std::move(R));
	};
	auto Slider = [&Out](const std::string& Label, const std::string& Help, int Impact, int Min, int Max, int Step, const std::string& Suffix, std::function<int()> Get, std::function<void(int)> Set) {
		SettingRow R;
		R.Label = Label;
		R.Help = Help;
		R.Impact = Impact;
		R.Slider = true;
		R.Min = Min;
		R.Max = Max;
		R.Step = Step;
		R.Suffix = Suffix;
		R.Get = std::move(Get);
		R.Set = std::move(Set);
		Out.push_back(std::move(R));
	};
	const std::vector<std::string> OffOn = {"OFF", "ON"};
	switch (ForTab)
	{
	case 0:
	{
		Choice("Quality preset",
			"Sets shadows, global illumination, reflections, textures and effects together. Epic suits an RTX 4070-class GPU at 1440p. Cinematic pushes every setting to film quality.",
			3, {"LOW", "MEDIUM", "HIGH", "EPIC", "CINEMATIC"}, [&S] { return S.Quality; }, [&S](int V) { S.Quality = V; });
		Choice("Ray-traced lighting",
			"Hardware ray tracing for global illumination and reflections: neon and screen light bouncing around the room, true reflections in the wet glass and on the desk. Needs a ray-tracing GPU.",
			3, OffOn, [&S] { return S.RayTracing ? 1 : 0; }, [&S](int V) { S.RayTracing = V != 0; });
		Slider("Resolution scale",
			"Renders fewer pixels and rebuilds the rest with temporal super resolution. Lower it for a higher frame rate; 100% is native.",
			2, 50, 100, 5, "%", [&S] { return S.ResolutionScale; }, [&S](int V) { S.ResolutionScale = V; });
		static const int Caps[] = {30, 60, 120, 144, 165, 240, 0};
		Choice("Frame rate limit", "Caps the frame rate. Match your monitor's refresh rate for smooth, even frames.", -1, {"30", "60", "120", "144", "165", "240", "UNLIMITED"},
			[&S] {
				for (int I = 0; I < 7; ++I)
				{
					if (Caps[I] == S.FrameRateLimit)
					{
						return I;
					}
				}
				return 6;
			},
			[&S](int V) { S.FrameRateLimit = Caps[V]; });
		Choice("V-Sync", "Locks frames to the display's refresh to remove tearing, at the cost of a little input latency.", -1, OffOn, [&S] { return S.VSync ? 1 : 0; }, [&S](int V) { S.VSync = V != 0; });
		Choice("Motion blur", "Blurs fast camera movement. Off keeps the laptop text crisp while you look around.", 0, OffOn, [&S] { return S.MotionBlur ? 1 : 0; }, [&S](int V) { S.MotionBlur = V != 0; });
		Choice("Film grain", "Fine noise over the image, like a camera sensor at night.", 0, {"OFF", "SUBTLE", "STRONG"}, [&S] { return S.FilmGrain; }, [&S](int V) { S.FilmGrain = V; });
		Choice("Chromatic aberration", "Color fringing toward the edges of the frame. It grows when you tilt and fades when you lean in to the screen.", 0, OffOn,
			[&S] { return S.ChromaticAberration ? 1 : 0; }, [&S](int V) { S.ChromaticAberration = V != 0; });
		break;
	}
	case 1:
	{
		Choice("Window mode", "Fullscreen gives the lowest latency. Borderless makes switching windows instant.", -1, {"FULLSCREEN", "BORDERLESS", "WINDOWED"}, [&S] { return S.WindowMode; },
			[&S](int V) { S.WindowMode = V; });
		std::vector<std::string> Res = Info.Resolutions;
		if (Res.empty())
		{
			Res.push_back("NATIVE");
		}
		Choice("Resolution", "Output resolution. Native is sharpest; to gain frame rate, lower the resolution scale instead so the interface stays crisp.", -1, Res,
			[&S, Res] {
				for (size_t I = 0; I < Res.size(); ++I)
				{
					if (Res[I] == S.Resolution)
					{
						return static_cast<int>(I);
					}
				}
				return 0;
			},
			[&S, Res](int V) { S.Resolution = Res[static_cast<size_t>(V)] == "NATIVE" ? std::string() : Res[static_cast<size_t>(V)]; });
		Slider("Brightness", "Overall exposure. Set it so the far corner of the room is barely visible, not black.", -1, 0, 100, 5, "", [&S] { return S.Brightness; },
			[&S](int V) { S.Brightness = V; });
		Slider("Field of view", "Vertical field of view while you sit back and look around. Leaning in always frames the laptop screen.", -1, 40, 70, 1, "\xC2\xB0", [&S] { return S.FieldOfView; },
			[&S](int V) { S.FieldOfView = V; });
		break;
	}
	case 2:
		Slider("Master volume", "Everything you hear.", -1, 0, 100, 5, "", [&S] { return S.MasterVolume; }, [&S](int V) { S.MasterVolume = V; });
		Slider("Effects", "Chips, cards, the laptop and the phone.", -1, 0, 100, 5, "", [&S] { return S.EffectsVolume; }, [&S](int V) { S.EffectsVolume = V; });
		Slider("Ambience", "Rain on the window, thunder and the city at night.", -1, 0, 100, 5, "", [&S] { return S.AmbienceVolume; }, [&S](int V) { S.AmbienceVolume = V; });
		Choice("Sound in background", "Keeps playing when another window is in front: a second screen, a stream, a chat.", -1, OffOn,
			[&S] { return S.BackgroundAudio ? 1 : 0; }, [&S](int V) { S.BackgroundAudio = V != 0; });
		break;
	default:
		Slider("Look sensitivity", "How far the view turns as you move the mouse while sitting back.", -1, 20, 200, 10, "%", [&S] { return S.LookSensitivity; },
			[&S](int V) { S.LookSensitivity = V; });
		Choice("Invert look", "Moving the mouse up looks down.", -1, OffOn, [&S] { return S.InvertLook ? 1 : 0; }, [&S](int V) { S.InvertLook = V != 0; });
		Choice("Control hints", "Shows the controls reminder after you sit down at the laptop.", -1, OffOn, [&S] { return S.ShowHints ? 1 : 0; }, [&S](int V) { S.ShowHints = V != 0; });
		break;
	}
	return Out;
}

void FrontEnd::ChangeSetting(int RowIndex, int Delta, bool Wrap)
{
	const std::vector<SettingRow> List = Rows(SelTab);
	if (RowIndex < 0 || RowIndex >= static_cast<int>(List.size()))
	{
		return;
	}
	const SettingRow& R = List[static_cast<size_t>(RowIndex)];
	const int Old = R.Get();
	int V = R.Slider ? Old + Delta * R.Step : Old + Delta;
	if (Wrap && !R.Slider)
	{
		const int N = R.Max - R.Min + 1;
		V = R.Min + ((V - R.Min) % N + N) % N;
	}
	V = std::clamp(V, R.Min, R.Max);
	if (V != Old)
	{
		R.Set(V);
		Hooks.SettingsChanged(Settings);
		Sound(SoundId::Click, 0.4);
	}
}

// ------------------------------------------------------------------ drawing

bool FrontEnd::Released(const Rect& R) const
{
	return Interactive && Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY);
}

bool FrontEnd::Button(const Rect& R, const std::string& Label, bool Primary, bool Focused)
{
	const bool Over = Interactive && Inside(R, Ptr.X, Ptr.Y);
	const float Lit = Over || Focused ? 1.0f : 0.0f;
	if (Primary)
	{
		C->FillRect(R, Paint(F(Mix(MenuNeon, Hex(0xff5aa3), 0.35f * Lit))));
		C->GlowRoundRect(R, 1.0f, F(MenuNeon, 0.35f + 0.25f * Lit), 18.0f);
		TrackedText(*C, Label, R.X + R.W * 0.5f, R.Y + R.H * 0.5f + 7.0f, 20.0f, 900, F(Hex(0x14040b)), 3.0f, Align::Center);
	}
	else
	{
		C->FillRect(R, Paint(F(MenuInk, 0.04f + 0.06f * Lit)));
		C->StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 1.0f, F(MenuInk, 0.28f + 0.3f * Lit), 1.5f);
		TrackedText(*C, Label, R.X + R.W * 0.5f, R.Y + R.H * 0.5f + 7.0f, 20.0f, 900, F(MenuInk, 0.8f + 0.2f * Lit), 3.0f, Align::Center);
	}
	return Released(R);
}

void FrontEnd::Scrim(Page P)
{
	const float H = Height;
	switch (P)
	{
	case Page::Attract:
		C->FillRect({0.0f, 0.0f, ViewW, H}, Paint::Radial({ViewW * 0.5f, H * 0.5f}, H * 0.25f, {ViewW * 0.5f, H * 0.5f}, ViewW * 0.72f, F(MenuShade, 0.0f), 0.55f, F(MenuShade, 0.35f), F(MenuShade, 0.85f)));
		break;
	case Page::Pause:
	case Page::Settings:
		if (P == Page::Pause || InGame)
		{
			C->FillRect({0.0f, 0.0f, ViewW, H}, Paint(F(MenuShade, P == Page::Pause ? 0.35f : 0.55f)));
		}
		else
		{
			C->FillRect({0.0f, 0.0f, ViewW, H}, Paint(F(MenuShade, 0.45f)));
		}
		[[fallthrough]];
	default:
		C->FillRect({0.0f, 0.0f, ViewW * 0.66f, H}, Paint::Linear({0.0f, 0.0f}, {ViewW * 0.66f, 0.0f}, F(MenuShade, 0.9f), F(MenuShade, 0.0f)));
		C->FillRect({0.0f, H - 260.0f, ViewW, 260.0f}, Paint::Linear({0.0f, H - 260.0f}, {0.0f, H}, F(MenuShade, 0.0f), F(MenuShade, 0.75f)));
		C->FillRect({0.0f, 0.0f, ViewW, 180.0f}, Paint::Linear({0.0f, 0.0f}, {0.0f, 180.0f}, F(MenuShade, 0.55f), F(MenuShade, 0.0f)));
		break;
	}
}

void FrontEnd::Logo(float X, float Y, float Size, bool Centered, double Now)
{
	const float Track = Size * 0.045f;
	const float Gap = Size * 0.3f;
	const float Ws = TrackedWidth(*C, "SHORT", Size, 900, Track);
	const float Wt = TrackedWidth(*C, "STACK", Size, 900, Track);
	const float X0 = Centered ? X - (Ws + Gap + Wt) * 0.5f : X;
	const float Level = NeonLevel(Centered ? PageTime : Now, Centered && Cur == Page::Attract);
	const float Sx = X0 + Ws + Gap;
	const float Gx = Sx + Wt * 0.5f;
	const float Gy = Y - Size * 0.36f;
	// Neon bloom: soft elliptical falloff (a radial gradient on a circle squashed vertically).
	auto Bloom = [&](float Rx, float Ry, float Alpha) {
		C->Save();
		C->Translate(Gx, Gy);
		C->Scale(1.0f, Ry / Rx);
		C->FillCircle(0.0f, 0.0f, Rx, Paint::Radial({0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, Rx, F(MenuNeon, Alpha * Level), 0.4f, F(MenuNeon, Alpha * 0.35f * Level), F(MenuNeon, 0.0f)));
		C->Restore();
	};
	Bloom(Wt * 0.95f, Size * 1.1f, 0.2f);
	Bloom(Wt * 0.6f, Size * 0.55f, 0.16f);
	// Anamorphic streak through the lit word, like a lens catching the sign.
	auto Streak = [&](float Half, float Thick, float Alpha) {
		Paint P;
		P.Type = Paint::Kind::Linear;
		P.P0 = {Gx - Half, 0.0f};
		P.P1 = {Gx + Half, 0.0f};
		P.C0 = F(MenuNeon, 0.0f);
		P.C1 = F(Hex(0xff8cc0), Alpha * Level);
		P.C2 = F(MenuNeon, 0.0f);
		P.Mid = 0.5f;
		C->FillRect({Gx - Half, Gy - Thick * 0.5f, Half * 2.0f, Thick}, P);
	};
	Streak(Wt * 1.9f, 1.5f, 0.55f);
	Streak(Wt * 1.3f, 7.0f, 0.12f);
	TrackedText(*C, "SHORT", X0, Y, Size, 900, F(MenuInk), Track);
	TrackedText(*C, "STACK", Sx, Y, Size, 900, F(Mix(Hex(0x4a1830), MenuNeon, Level)), Track);
	// A thin hot core makes the tube read as lit.
	TrackedText(*C, "STACK", Sx, Y, Size, 900, F(Hex(0xffc2dc), 0.22f * Level), Track);

	const float Sub = std::max(15.0f, Size * 0.15f);
	const float SubY = Y + Size * 0.5f;
	if (Centered)
	{
		const float Tw = TrackedWidth(*C, "NIGHT ONE", Sub, 700, Sub * 0.45f);
		C->FillRect({X - Tw * 0.5f - 86.0f, SubY - Sub * 0.38f, 60.0f, 2.0f}, Paint(F(MenuNeon, 0.8f)));
		C->FillRect({X + Tw * 0.5f + 26.0f, SubY - Sub * 0.38f, 60.0f, 2.0f}, Paint(F(MenuNeon, 0.8f)));
		TrackedText(*C, "NIGHT ONE", X, SubY, Sub, 700, F(MenuMuted), Sub * 0.45f, Align::Center);
	}
	else
	{
		C->FillRect({X0 + 2.0f, SubY - Sub * 0.4f, 44.0f, 3.0f}, Paint(F(MenuNeon)));
		TrackedText(*C, "NIGHT ONE", X0 + 62.0f, SubY, Sub, 700, F(MenuMuted), Sub * 0.45f);
	}
}

void FrontEnd::MenuList(const std::vector<MenuItem>& List, float X, float Y, double Now)
{
	const float Step = 68.0f;
	if (Hot.size() < List.size())
	{
		Hot.resize(List.size(), 0.0f);
	}
	for (size_t I = 0; I < List.size(); ++I)
	{
		const MenuItem& It = List[I];
		const float Yi = Y + Step * static_cast<float>(I);
		const Rect Hit = {X - 40.0f, Yi - 48.0f, 640.0f, 62.0f};
		const int Idx = static_cast<int>(I);
		if (Interactive && Confirm == Modal::None && It.Enabled && PtrMoved && Inside(Hit, Ptr.X, Ptr.Y) && Sel != Idx)
		{
			Sel = Idx;
			SelAt = Now;
			Sound(SoundId::Click, 0.25);
		}
		if (Confirm == Modal::None && It.Enabled && Released(Hit))
		{
			Activate(Idx, Now);
			return;
		}
		const float Target = Idx == Sel ? 1.0f : 0.0f;
		Hot[I] += (Target - Hot[I]) * static_cast<float>(std::min(1.0, FrameDt * 12.0));
		const float Hi = Hot[I];
		const float In = Ease((PageTime - 0.06 - 0.05 * static_cast<double>(I)) / 0.45);
		const float Tx = X - (1.0f - In) * 48.0f + Hi * 20.0f;
		if (Hi > 0.01f)
		{
			C->FillRect({X - 40.0f, Yi - 46.0f, 660.0f, 58.0f}, Paint::Linear({X - 40.0f, 0.0f}, {X + 620.0f, 0.0f}, F(MenuNeon, 0.2f * Hi * In), F(MenuNeon, 0.0f)));
			C->GlowRoundRect({X - 40.0f, Yi - 40.0f, 4.0f, 46.0f}, 1.0f, F(MenuNeon, 0.55f * Hi * In), 12.0f);
			C->FillRect({X - 40.0f, Yi - 40.0f, 4.0f, 46.0f}, Paint(F(MenuNeon, Hi * In)));
		}
		const Color Col = It.Enabled ? Mix(MenuMuted, MenuInk, Hi) : MenuDim;
		const float A = (It.Enabled ? 0.66f + 0.34f * Hi : 0.5f) * In;
		const float Lw = TrackedWidth(*C, It.Label, 40.0f, 900, 2.5f);
		TrackedText(*C, It.Label, Tx, Yi, 40.0f, 900, F(Col, A), 2.5f);
		if (!It.Enabled)
		{
			TrackedText(*C, "NO SAVE YET", Tx + Lw + 22.0f, Yi - 16.0f, 12.0f, 700, F(MenuDim, 0.9f * In), 2.5f);
		}
	}
}

float FrontEnd::ContextBody(float X, float Y, float CardW)
{
	const float Pad = 36.0f;
	const float Inner = CardW - Pad * 2.0f;
	float Yc = Y + Pad + 14.0f;
	const float A = Ease((LastNow - SelAt) / 0.3);
	auto Label = [&](const std::string& S, const Color& Col) {
		if (CardPaint)
		{
			TrackedText(*C, S, X + Pad, Yc, 13.0f, 700, F(Col, A), 3.2f);
		}
		Yc += 44.0f;
	};
	auto Title = [&](const std::string& S) {
		if (CardPaint)
		{
			C->Text(S, X + Pad, Yc, Ts(36.0f, 900, F(MenuInk, A), Align::Left, Baseline::Alphabetic, false, Inner));
		}
		Yc += 22.0f;
	};
	auto Para = [&](const std::string& S, const Color& Col) {
		Yc += 12.0f;
		for (const std::string& L : WrapText(*C, S, Inner, 19.0f, 300))
		{
			Yc += 29.0f;
			if (CardPaint)
			{
				C->Text(L, X + Pad, Yc, Ts(19.0f, 300, F(Col, 0.92f * A)));
			}
		}
	};
	auto Row = [&](const std::string& Name, const std::string& Value, const Color& ValueCol) {
		Yc += 40.0f;
		if (CardPaint)
		{
			C->FillRect({X + Pad, Yc + 13.0f, Inner, 1.0f}, Paint(F(MenuInk, 0.07f * A)));
			TrackedText(*C, Name, X + Pad, Yc, 13.0f, 700, F(MenuMuted, A), 2.4f);
			C->Text(Value, X + CardW - Pad, Yc, Ts(19.0f, 700, F(ValueCol, A), Align::Right));
		}
	};
	auto Warning = [&](const std::string& S) {
		Yc += 16.0f;
		const std::vector<std::string> Lines = WrapText(*C, S, Inner - 34.0f, 17.0f, 600);
		for (size_t I = 0; I < Lines.size(); ++I)
		{
			Yc += 26.0f;
			if (CardPaint)
			{
				if (I == 0)
				{
					const float Cx = X + Pad + 10.0f;
					const float Cy = Yc - 6.0f;
					C->FillPolygon({{Cx, Cy - 10.0f}, {Cx + 10.0f, Cy + 8.0f}, {Cx - 10.0f, Cy + 8.0f}}, Paint(F(MenuWarn, A)));
					C->Text("!", Cx, Cy + 6.0f, Ts(12.0f, 900, F(Hex(0x14040b), A), Align::Center));
				}
				C->Text(Lines[I], X + Pad + 34.0f, Yc, Ts(17.0f, 600, F(MenuWarn, A)));
			}
		}
	};

	const bool Pause = Cur == Page::Pause || (Cur == Page::Hidden && Prev == Page::Pause);
	if (Cur == Page::NewGame)
	{
		Label("YOUR SITUATION", MenuTeal);
		Title("2:07 AM");
		Row("BANKROLL", "$2.37", MenuGold);
		Row("RENT DUE FRIDAY", "$1,225.00", MenuWarn);
		Row("ON THE DOOR", "FINAL NOTICE", MenuInk);
		Para("Tonight on RiverLine: the Night Owl Turbo, $1.10 to enter, a thousand players, $178 to the winner.", MenuInk);
	}
	else if (Pause)
	{
		switch (Sel)
		{
		case 0:
			Label("TONIGHT", MenuTeal);
			Title(Info.HeroName);
			for (const std::pair<std::string, std::string>& Kv : Info.Status)
			{
				Row(Kv.first, Kv.second, Kv.first == "BANKROLL" ? MenuGold : MenuInk);
			}
			break;
		case 1:
			Label("SETTINGS", MenuTeal);
			Title("Graphics, audio, controls");
			Para("Changes apply as you make them.", MenuInk);
			break;
		default:
			Label(Sel == 2 ? "MAIN MENU" : "DESKTOP", MenuTeal);
			Title(Sel == 2 ? "Leave the table" : "Log off for the night");
			Para("Your bankroll and career are saved.", MenuInk);
			if (Info.InTournament)
			{
				Warning("You will leave the tournament in progress. The buy-in is not refunded.");
			}
			break;
		}
	}
	else
	{
		switch (Sel)
		{
		case 0:
			Label("CONTINUE CAREER", MenuTeal);
			Title(Info.HeroName);
			if (Info.Person.Created)
			{
				Row("PLAYING AS", Info.Person.FullName() + " \xC2\xB7 " + hero::InfoOf(Info.Person.Story).Name, MenuInk);
			}
			Row("BANKROLL", Money(Info.BankrollCents), MenuGold);
			Row("TOURNAMENTS PLAYED", std::to_string(Info.Tournaments), MenuInk);
			if (!Info.BestFinish.empty())
			{
				Row("BEST FINISH", Info.BestFinish, MenuInk);
			}
			if (!Info.LastResult.empty())
			{
				Row("LAST EVENT", Info.LastResult, MenuInk);
			}
			Warning("Rent is due Friday: $1,225.");
			break;
		case 1:
			Label("NEW CAREER", MenuTeal);
			Title("Night One");
			Para("Rain on the window, $2.37 in your RiverLine account and a final notice on the door. Rent is due Friday.", MenuInk);
			if (Info.HasSave)
			{
				Warning("Starting over replaces your current career.");
			}
			break;
		case 2:
			Label("SETTINGS", MenuTeal);
			Title("Graphics, audio, controls");
			Para("Tuned for high-end PCs: ray-traced lighting is on by default, and the Cinematic preset is there when you want every last detail.", MenuInk);
			break;
		case 3:
			Label("CREDITS", MenuTeal);
			Title("Short Stack");
			Para("The people and tools behind the game.", MenuInk);
			break;
		default:
			Label("QUIT GAME", MenuTeal);
			Title("Log off for the night");
			Para("Your bankroll and career are saved.", MenuInk);
			break;
		}
	}
	return Yc - Y + Pad;
}

void FrontEnd::ContextCard(float X, float Y, float CardW, double Now)
{
	(void)Now;
	CardPaint = false;
	const float CardH = ContextBody(X, Y, CardW);
	CardPaint = true;
	const float A = Ease((LastNow - SelAt) / 0.3);
	const float Dy = (1.0f - A) * 10.0f;
	C->FillRect({X, Y + Dy, CardW, CardH}, Paint(F(MenuPanel, 0.78f)));
	C->StrokeRoundRect({X + 0.5f, Y + Dy + 0.5f, CardW - 1.0f, CardH - 1.0f}, 1.0f, F(MenuInk, 0.08f), 1.0f);
	C->FillRect({X, Y + Dy, 56.0f * A, 3.0f}, Paint(F(MenuNeon)));
	ContextBody(X, Y + Dy, CardW);
}

void FrontEnd::Hints(const std::vector<std::pair<std::string, std::string>>& Pairs)
{
	float X = MenuMargin;
	const float Y = Height - 62.0f;
	for (const std::pair<std::string, std::string>& P : Pairs)
	{
		// "UP|DOWN/DPAD": keyboard glyphs before the slash, gamepad glyphs after.
		const size_t Slash = P.first.find('/');
		const std::string Set = Gamepad && Slash != std::string::npos ? P.first.substr(Slash + 1) : P.first.substr(0, Slash);
		size_t Start = 0;
		while (Start <= Set.size())
		{
			size_t End = Set.find('|', Start);
			if (End == std::string::npos)
			{
				End = Set.size();
			}
			X += Glyph(*C, Set.substr(Start, End - Start), X, Y, Fade) + 6.0f;
			Start = End + 1;
		}
		X += 6.0f;
		X += TrackedText(*C, P.second, X, Y - 2.0f, 13.0f, 700, F(MenuMuted), 2.6f) + 34.0f;
	}
}

void FrontEnd::Footer()
{
	TrackedText(*C, "SHORT STACK " + Info.Version + " \xC2\xB7 PROTOTYPE BUILD", ViewW - MenuMargin, Height - 64.0f, 12.0f, 700, F(MenuDim), 2.4f, Align::Right);
}

void FrontEnd::AttractPage(double Now)
{
	const float H = Height;
	const float Bar = 112.0f;
	C->FillRect({0.0f, 0.0f, ViewW, Bar}, Paint(F(Hex(0x000000))));
	C->FillRect({0.0f, H - Bar, ViewW, Bar}, Paint(F(Hex(0x000000))));
	Logo(ViewW * 0.5f, H * 0.5f + 20.0f, 150.0f, true, Now);
	if (PageTime > 1.2)
	{
		const float Pulse = 0.4f + 0.6f * (0.5f + 0.5f * static_cast<float>(std::sin((PageTime - 1.2) * 2.4)));
		const float In = Ease((PageTime - 1.2) / 0.8);
		if (Gamepad)
		{
			const float Wd = TrackedWidth(*C, "PRESS", 19.0f, 700, 8.0f) + 44.0f;
			const float X0 = ViewW * 0.5f - Wd * 0.5f;
			TrackedText(*C, "PRESS", X0, H - 250.0f, 19.0f, 700, F(MenuInk, Pulse * In), 8.0f);
			Glyph(*C, "A", X0 + Wd - 28.0f, H - 245.0f, Fade * Pulse * In);
		}
		else
		{
			TrackedText(*C, "PRESS ANY KEY", ViewW * 0.5f, H - 250.0f, 19.0f, 700, F(MenuInk, Pulse * In), 8.0f, Align::Center);
		}
	}
	TrackedText(*C, "A FIRST-PERSON POKER RPG", MenuMargin, H - Bar * 0.5f + 5.0f, 12.0f, 700, F(MenuMuted, 0.8f), 3.2f);
	TrackedText(*C, Info.Version + " \xC2\xB7 PROTOTYPE BUILD", ViewW - MenuMargin, H - Bar * 0.5f + 5.0f, 12.0f, 700, F(MenuDim), 2.4f, Align::Right);
	if (Interactive && Ptr.Released)
	{
		Key("Any", Now);
	}
}

void FrontEnd::MainPage(double Now)
{
	if (Prev == Page::Attract && PageTime < 0.8)
	{
		const float Bar = 112.0f * (1.0f - Ease(PageTime / 0.8));
		C->FillRect({0.0f, 0.0f, ViewW, Bar}, Paint(Hex(0x000000)));
		C->FillRect({0.0f, Height - Bar, ViewW, Bar}, Paint(Hex(0x000000)));
	}
	Logo(MenuMargin, 212.0f, 92.0f, false, Now);
	MenuList(Items(), MenuMargin + 4.0f, 470.0f, Now);
	const float CardW = 540.0f;
	ContextCard(ViewW - MenuMargin - CardW, 410.0f, CardW, Now);
	Hints({{"UP|DOWN/DPAD", "NAVIGATE"}, {"ENTER/A", "SELECT"}, {"ESC/B", "QUIT"}});
	Footer();
}

void FrontEnd::PausePage(double Now)
{
	TrackedText(*C, "PAUSED", MenuMargin, 268.0f, 84.0f, 900, F(MenuInk), 4.0f);
	std::string Clock;
	for (const std::pair<std::string, std::string>& Kv : Info.Status)
	{
		Clock = Kv.first == "TIME" ? Kv.second : Clock;
	}
	C->FillRect({MenuMargin + 2.0f, 310.0f, 44.0f, 3.0f}, Paint(F(MenuNeon)));
	TrackedText(*C, "NIGHT ONE" + (Clock.empty() ? std::string() : " \xC2\xB7 " + Clock), MenuMargin + 62.0f, 318.0f, 15.0f, 700, F(MenuMuted), 6.0f);
	MenuList(Items(), MenuMargin + 4.0f, 470.0f, Now);
	const float CardW = 540.0f;
	ContextCard(ViewW - MenuMargin - CardW, 410.0f, CardW, Now);
	Hints({{"UP|DOWN/DPAD", "NAVIGATE"}, {"ENTER/A", "SELECT"}, {"ESC/B", "RESUME"}});
	Footer();
}

void FrontEnd::SettingsPage(double Now)
{
	const float X = MenuMargin;
	TrackedText(*C, "SETTINGS", X, 200.0f, 84.0f, 900, F(MenuInk), 4.0f);

	// Tabs.
	static const char* const Tabs[] = {"GRAPHICS", "DISPLAY", "AUDIO", "CONTROLS"};
	float Tx = X;
	const float TabY = 280.0f;
	Tx += Glyph(*C, Gamepad ? "LB" : "Q", Tx, TabY + 3.0f, Fade) + 26.0f;
	for (int I = 0; I < 4; ++I)
	{
		const float Tw = TrackedWidth(*C, Tabs[I], 18.0f, 900, 3.4f);
		const Rect Hit = {Tx - 12.0f, TabY - 30.0f, Tw + 24.0f, 46.0f};
		const bool Over = Interactive && Inside(Hit, Ptr.X, Ptr.Y);
		if (Released(Hit) && SelTab != I)
		{
			SelTab = I;
			SelRow = 0;
			SelAt = Now;
			Sound(SoundId::Click, 0.4);
		}
		const bool Active = SelTab == I;
		TrackedText(*C, Tabs[I], Tx, TabY, 18.0f, 900, F(Active ? MenuInk : MenuMuted, Active ? 1.0f : Over ? 0.95f : 0.7f), 3.4f);
		if (Active)
		{
			C->FillRect({Tx, TabY + 14.0f, Tw, 3.0f}, Paint(F(MenuNeon)));
		}
		Tx += Tw + 44.0f;
	}
	Glyph(*C, Gamepad ? "RB" : "E", Tx - 18.0f, TabY + 3.0f, Fade);
	C->FillRect({X, TabY + 17.0f, std::min(960.0f, ViewW - 2.0f * X), 1.0f}, Paint(F(MenuInk, 0.1f)));

	// Rows.
	const std::vector<SettingRow> List = Rows(SelTab);
	const float RowW = std::min(900.0f, ViewW - 2.0f * X - 560.0f);
	const float Y0 = 368.0f;
	const float Step = 62.0f;
	if (Hot.size() < List.size())
	{
		Hot.resize(List.size(), 0.0f);
	}
	if (!Ptr.Down)
	{
		SliderDrag = false;
	}
	for (size_t I = 0; I < List.size(); ++I)
	{
		const SettingRow& R = List[I];
		const int Idx = static_cast<int>(I);
		const float Yr = Y0 + Step * static_cast<float>(I);
		const Rect Band = {X - 24.0f, Yr - 38.0f, RowW + 48.0f, 54.0f};
		if (Interactive && PtrMoved && !SliderDrag && Inside(Band, Ptr.X, Ptr.Y) && SelRow != Idx)
		{
			SelRow = Idx;
			SelAt = Now;
			Sound(SoundId::Click, 0.2);
		}
		const float Target = Idx == SelRow ? 1.0f : 0.0f;
		Hot[I] += (Target - Hot[I]) * static_cast<float>(std::min(1.0, FrameDt * 14.0));
		const float Hi = Hot[I];
		const float In = Ease((PageTime - 0.04 * static_cast<double>(I)) / 0.4);
		if (Hi > 0.01f)
		{
			C->FillRect(Band, Paint::Linear({Band.X, 0.0f}, {Band.X + Band.W, 0.0f}, F(MenuNeon, 0.15f * Hi * In), F(MenuNeon, 0.03f * Hi * In)));
			C->FillRect({Band.X, Band.Y, 3.0f, Band.H}, Paint(F(MenuNeon, Hi * In)));
		}
		C->Text(R.Label, X + 8.0f, Yr, Ts(22.0f, 500, F(Mix(MenuMuted, MenuInk, 0.35f + 0.65f * Hi), In)));

		const float Rx = X + RowW;
		const int V = R.Get();
		if (R.Slider)
		{
			const float T0 = Rx - 330.0f;
			const float T1 = Rx - 80.0f;
			const Rect Track = {T0 - 10.0f, Yr - 26.0f, T1 - T0 + 20.0f, 36.0f};
			if (Interactive && Ptr.Pressed && Inside(Track, Ptr.X, Ptr.Y))
			{
				SliderDrag = true;
				SelRow = Idx;
			}
			if (SliderDrag && SelRow == Idx && Ptr.Down)
			{
				const float K = std::clamp((Ptr.X - T0) / (T1 - T0), 0.0f, 1.0f);
				int Nv = R.Min + static_cast<int>(std::lround(K * static_cast<float>(R.Max - R.Min) / static_cast<float>(R.Step))) * R.Step;
				Nv = std::clamp(Nv, R.Min, R.Max);
				if (Nv != V)
				{
					ChangeSetting(Idx, (Nv - V) / R.Step, false);
				}
			}
			const float K = static_cast<float>(V - R.Min) / static_cast<float>(std::max(1, R.Max - R.Min));
			C->FillRect({T0, Yr - 9.0f, T1 - T0, 4.0f}, Paint(F(MenuInk, 0.14f * In)));
			C->FillRect({T0, Yr - 9.0f, (T1 - T0) * K, 4.0f}, Paint(F(MenuNeon, In)));
			C->FillCircle(T0 + (T1 - T0) * K, Yr - 7.0f, 8.0f + 2.0f * Hi, Paint(F(MenuInk, In)));
			C->Text(std::to_string(V) + R.Suffix, Rx, Yr, Ts(20.0f, 700, F(MenuInk, In), Align::Right));
		}
		else
		{
			const float Cx = Rx - 160.0f;
			const Rect LeftHit = {Rx - 340.0f, Yr - 30.0f, 60.0f, 44.0f};
			const Rect RightHit = {Rx - 40.0f, Yr - 30.0f, 60.0f, 44.0f};
			const Rect Middle = {Rx - 280.0f, Yr - 30.0f, 240.0f, 44.0f};
			if (Released(LeftHit))
			{
				SelRow = Idx;
				ChangeSetting(Idx, -1, false);
			}
			else if (Released(RightHit))
			{
				SelRow = Idx;
				ChangeSetting(Idx, 1, false);
			}
			else if (Released(Middle))
			{
				SelRow = Idx;
				ChangeSetting(Idx, 1, true);
			}
			const int Nv = R.Get();
			const std::string& Label = R.Options[static_cast<size_t>(std::clamp(Nv, 0, R.Max))];
			TrackedText(*C, Label, Cx, Yr - 2.0f, 19.0f, 900, F(MenuInk, In), 2.0f, Align::Center);
			Chevron(*C, Rx - 310.0f, Yr - 8.0f, 8.0f, false, F(MenuInk, (Nv > R.Min ? 0.35f + 0.5f * Hi : 0.12f) * In));
			Chevron(*C, Rx - 10.0f, Yr - 8.0f, 8.0f, true, F(MenuInk, (Nv < R.Max ? 0.35f + 0.5f * Hi : 0.12f) * In));
			const int N = R.Max - R.Min + 1;
			if (N <= 8)
			{
				const float Pw = 16.0f;
				const float Px0 = Cx - (Pw * static_cast<float>(N) + 6.0f * static_cast<float>(N - 1)) * 0.5f;
				for (int K = 0; K < N; ++K)
				{
					C->FillRect({Px0 + (Pw + 6.0f) * static_cast<float>(K), Yr + 9.0f, Pw, 3.0f}, Paint(F(K == Nv - R.Min ? MenuNeon : MenuInk, (K == Nv - R.Min ? 1.0f : 0.14f) * In)));
				}
			}
		}
	}

	// Description of the selected row.
	if (SelRow >= 0 && SelRow < static_cast<int>(List.size()))
	{
		const SettingRow& R = List[static_cast<size_t>(SelRow)];
		const float Dx = X + RowW + 96.0f;
		const float Dw = std::min(500.0f, ViewW - MenuMargin - Dx);
		if (Dw > 240.0f)
		{
			const float A = Ease((Now - SelAt) / 0.25);
			float Yd = Y0 - 8.0f;
			C->FillRect({Dx, Yd - 30.0f, 44.0f, 3.0f}, Paint(F(MenuNeon, A)));
			for (const std::string& L : WrapText(*C, R.Label, Dw, 32.0f, 900))
			{
				Yd += 12.0f;
				C->Text(L, Dx, Yd, Ts(32.0f, 900, F(MenuInk, A)));
				Yd += 26.0f;
			}
			Yd += 8.0f;
			for (const std::string& L : WrapText(*C, R.Help, Dw, 19.0f, 300))
			{
				Yd += 30.0f;
				C->Text(L, Dx, Yd, Ts(19.0f, 300, F(MenuInk, 0.88f * A)));
			}
			if (R.Impact >= 0)
			{
				Yd += 54.0f;
				TrackedText(*C, "GPU COST", Dx, Yd, 13.0f, 700, F(MenuMuted, A), 3.2f);
				static const uint32_t Heat[] = {0x27d3c3u, 0xf2c14eu, 0xf28a3au, 0xff5a5fu};
				for (int K = 0; K < 4; ++K)
				{
					const bool On = K <= R.Impact;
					C->FillRect({Dx + 130.0f + 34.0f * static_cast<float>(K), Yd - 12.0f, 28.0f, 6.0f}, Paint(F(On ? Hex(Heat[R.Impact]) : MenuInk, (On ? 1.0f : 0.14f) * A)));
				}
			}
		}
	}
	Hints({{"UP|DOWN/DPAD", "SELECT"}, {"LEFT|RIGHT/DPAD", "CHANGE"}, {"Q|E/LB|RB", "TAB"}, {"R/Y", "DEFAULTS"}, {"ESC/B", "BACK"}});
	Footer();
}

void FrontEnd::CreditsPage(double Now)
{
	const float X = MenuMargin;
	TrackedText(*C, "CREDITS", X, 200.0f, 84.0f, 900, F(MenuInk), 4.0f);
	const Rect Clip = {X - 20.0f, 250.0f, std::min(1000.0f, ViewW - 2.0f * X), Height - 250.0f - 130.0f};
	if (Now - CreditsManualAt > 3.0)
	{
		CreditsOffset += FrameDt * 34.0;
	}
	if (Ptr.Wheel != 0.0f)
	{
		CreditsOffset = std::max(0.0, CreditsOffset + static_cast<double>(Ptr.Wheel));
		CreditsManualAt = Now;
	}
	float Total = 0.0f;
	for (const char* Line : CreditLines)
	{
		Total += Line[0] == '\0' ? 40.0f : Line[0] == '*' ? 90.0f : Line[0] == '#' ? 50.0f : Line[0] == '~' ? 30.0f : 46.0f;
	}
	const double Loop = static_cast<double>(Total + Clip.H);
	if (CreditsOffset > Loop)
	{
		CreditsOffset = std::fmod(CreditsOffset, Loop);
	}
	C->PushClip(Clip);
	float Y = Clip.Y + Clip.H * 0.35f - static_cast<float>(CreditsOffset);
	for (const char* Line : CreditLines)
	{
		const std::string S = Line;
		if (S.empty())
		{
			Y += 40.0f;
			continue;
		}
		const std::string Body = S.substr(S[0] == '*' || S[0] == '#' || S[0] == '~' ? 1 : 0);
		const float Edge = std::min(1.0f, std::min(Y - Clip.Y, Clip.Y + Clip.H - Y) / 90.0f);
		const float A = std::max(0.0f, Edge);
		switch (S[0])
		{
		case '*':
			Y += 90.0f;
			TrackedText(*C, Body, X, Y, 64.0f, 900, F(MenuInk, A), 4.0f);
			break;
		case '#':
			Y += 50.0f;
			TrackedText(*C, Body, X, Y, 13.0f, 700, F(MenuNeon, A), 4.0f);
			break;
		case '~':
			Y += 30.0f;
			C->Text(Body, X, Y, Ts(18.0f, 300, F(MenuMuted, A)));
			break;
		default:
			Y += 46.0f;
			C->Text(Body, X, Y, Ts(32.0f, 300, F(MenuInk, A)));
			break;
		}
	}
	C->PopClip();
	Hints({{"UP|DOWN/DPAD", "SCROLL"}, {"ESC/B", "BACK"}});
	Footer();
}

void FrontEnd::ModalBox(double Now)
{
	const float A = Ease((Now - ConfirmAt) / 0.25);
	const float SavedFade = Fade;
	Fade = A;
	C->FillRect({0.0f, 0.0f, ViewW, Height}, Paint(F(MenuShade, 0.7f)));
	const float Mw = 720.0f;
	const float Mh = 300.0f;
	const float Mx = ViewW * 0.5f - Mw * 0.5f;
	const float My = Height * 0.5f - Mh * 0.5f + (1.0f - A) * 16.0f;
	C->FillRect({Mx, My, Mw, Mh}, Paint(F(MenuPanel, 0.96f)));
	C->StrokeRoundRect({Mx + 0.5f, My + 0.5f, Mw - 1.0f, Mh - 1.0f}, 1.0f, F(MenuInk, 0.1f), 1.0f);
	C->FillRect({Mx, My, 56.0f, 3.0f}, Paint(F(MenuNeon)));
	const bool ToMenu = Confirm == Modal::QuitToMenu;
	TrackedText(*C, ToMenu ? "QUIT TO MAIN MENU?" : "QUIT GAME?", Mx + 44.0f, My + 84.0f, 36.0f, 900, F(MenuInk), 2.0f);
	const std::string Body = Info.InTournament ? "You will leave the tournament in progress. The buy-in is not refunded." : "Your bankroll and career are saved.";
	float Yb = My + 104.0f;
	for (const std::string& L : WrapText(*C, Body, Mw - 88.0f, 20.0f, 300))
	{
		Yb += 30.0f;
		C->Text(L, Mx + 44.0f, Yb, Ts(20.0f, 300, F(Info.InTournament ? MenuWarn : MenuInk, 0.9f)));
	}
	const Rect Yes = {Mx + 44.0f, My + Mh - 100.0f, 200.0f, 60.0f};
	const Rect No = {Mx + 264.0f, My + Mh - 100.0f, 200.0f, 60.0f};
	if (Interactive && PtrMoved)
	{
		ConfirmSel = Inside(Yes, Ptr.X, Ptr.Y) ? 0 : Inside(No, Ptr.X, Ptr.Y) ? 1 : ConfirmSel;
	}
	const bool Saved = Interactive;
	const Modal Which = Confirm;
	if (Button(Yes, "QUIT", ConfirmSel == 0, ConfirmSel == 0))
	{
		Confirm = Modal::None;
		Sound(SoundId::Fold, 0.6);
		if (Which == Modal::QuitGame)
		{
			Hooks.QuitGame();
		}
		else
		{
			Hooks.QuitToMenu();
		}
	}
	else if (Button(No, "CANCEL", ConfirmSel == 1, ConfirmSel == 1))
	{
		Confirm = Modal::None;
		Sound(SoundId::Check, 0.4);
	}
	Interactive = Saved;
	Fade = SavedFade;
}

void FrontEnd::DrawPage(Page P, double Now)
{
	switch (P)
	{
	case Page::Attract: AttractPage(Now); break;
	case Page::Main: MainPage(Now); break;
	case Page::NewGame: NewGamePage(Now); break;
	case Page::Settings: SettingsPage(Now); break;
	case Page::Credits: CreditsPage(Now); break;
	case Page::Pause: PausePage(Now); break;
	default: break;
	}
}

void FrontEnd::Draw(Canvas& Cv, double Now)
{
	C = &Cv;
	ViewW = Cv.Width();
	FrameDt = std::clamp(Now - LastNow, 0.0, 0.1);
	LastNow = Now;
	PtrMoved = std::fabs(Ptr.X - LastPtrX) + std::fabs(Ptr.Y - LastPtrY) > 0.5f;
	LastPtrX = Ptr.X;
	LastPtrY = Ptr.Y;
	if (Ptr.Pressed)
	{
		PressX = Ptr.X;
		PressY = Ptr.Y;
	}
	const double Since = Now - PageAt;
	if (Cur == Page::Hidden)
	{
		// Fading out after Continue, New Game or Resume.
		if (Prev != Page::Hidden && Since < 0.6)
		{
			Interactive = false;
			Fade = 1.0f - Ease(Since / 0.6);
			PageTime = 10.0;
			Scrim(Prev);
			DrawPage(Prev, Now);
		}
	}
	else
	{
		const bool FromHidden = Prev == Page::Hidden;
		Fade = FromHidden ? Ease(Since / 0.6) : 1.0f;
		Scrim(Cur);
		const Page Drawn = Cur;
		Interactive = Confirm == Modal::None;
		Fade = Ease((Since - (FromHidden ? 0.2 : 0.04)) / 0.4);
		PageTime = Since;
		DrawPage(Drawn, Now);
		if (Confirm != Modal::None && Cur != Page::Hidden)
		{
			Interactive = true;
			ModalBox(Now);
		}
	}
	Ptr.EndFrame();
}
} // namespace ui
} // namespace ss
