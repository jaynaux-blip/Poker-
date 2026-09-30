#include "NightOneGame.h"

#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "NightOneGameMode.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"

FNightOneGame::FNightOneGame(ANightOneGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed)
	: Mode(InMode), Session(*this, Seed, Loaded), Client(Session)
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
	UNightOneSaveGame* SaveObject = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass()));
	if (SaveObject)
	{
		SaveObject->Data = FString(UTF8_TO_TCHAR(Data.Serialize().c_str()));
		UGameplayStatics::SaveGameToSlot(SaveObject, UNightOneSaveGame::SlotName(), 0);
	}
}
