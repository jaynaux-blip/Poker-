#include "NightOneGame.h"

#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "NightOneGameMode.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"

FNightOneGame::FNightOneGame(ANightOneGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed)
	: Mode(InMode), Session(*this, Seed, Loaded), Client(Session), Menu(*this)
{
}

void FNightOneGame::Sound(ss::SoundId Id, double Volume)
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->Play(Id, static_cast<float>(Volume));
	}
}

void FNightOneGame::Text(const std::string& From, const std::string& Body)
{
	Phone.Notify(From, Body, RealNow);
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->PlayEffect(ss::audio::Effect::Buzz);
	}
	Mode.ShowToast(FString(UTF8_TO_TCHAR(From.c_str())), FString(UTF8_TO_TCHAR(Body.c_str())));
}

void FNightOneGame::Heartbeat(bool bOn)
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->SetHeartbeat(bOn);
	}
}

void FNightOneGame::AddCan()
{
	if (ANightOneStage* S = Mode.GetStage())
	{
		S->AddCan();
	}
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->PlayEffect(ss::audio::Effect::CanOpen);
	}
}

void FNightOneGame::Celebrate()
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->Play(ss::SoundId::Cash);
		A->Play(ss::SoundId::Win);
	}
}

void FNightOneGame::Save(const ss::SaveData& Data)
{
	// The world's text, compression and the write happen on a worker (CareerSave.h): nothing here hitches.
	Saver.Submit(Data);
}

bool FNightOneGame::GoOut(const std::string& ActivityId, ss::Chips BuyInCents)
{
	return Mode.GoOut(FString(UTF8_TO_TCHAR(ActivityId.c_str())), static_cast<int64>(BuyInCents));
}

// ------------------------------------------------------------------ menus

void FNightOneGame::UiSound(ss::SoundId Id, double Volume)
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->Play(Id, static_cast<float>(Volume));
	}
}

void FNightOneGame::Continue()
{
	Mode.ContinueCareer();
}

void FNightOneGame::NewGame(const std::string& ScreenName)
{
	Mode.StartNewCareer(FString(UTF8_TO_TCHAR(ScreenName.c_str())));
}

void FNightOneGame::NewCareer(const std::string& ScreenName, const ss::hero::Character& Who)
{
	Mode.StartNewCareer(FString(UTF8_TO_TCHAR(ScreenName.c_str())), &Who);
}

void FNightOneGame::Resume()
{
	Mode.ResumePlay();
}

void FNightOneGame::QuitToMenu()
{
	Mode.QuitToMainMenu();
}

void FNightOneGame::QuitGame()
{
	Mode.QuitToDesktop();
}

void FNightOneGame::SettingsChanged(const ss::ui::GameSettings& Settings)
{
	Mode.ApplySettings(Settings, true);
}
