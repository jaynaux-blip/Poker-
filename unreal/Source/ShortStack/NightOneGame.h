#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/Phone.h"
#include "ShortStack/UI/RiverLine.h"
#include "SlateDrawList.h"

class ANightOneGameMode;

/** The game session, the menus, and their hooks into the world (sound, the phone, the desk, saves). */
class FNightOneGame final : public ss::SessionHooks, public ss::ui::FrontEndHooks
{
public:
	FNightOneGame(ANightOneGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed);

	ANightOneGameMode& Mode;
	ss::Session Session;
	ss::ui::RiverLine Client;
	ss::ui::PhoneScreen Phone;
	ss::ui::FrontEnd Menu;
	FSlateTextMeasurer Measurer;
	double RealNow = 0.0;

	// ss::SessionHooks
	virtual void Sound(ss::SoundId Id, double Volume) override;
	virtual void Text(const std::string& From, const std::string& Body) override;
	virtual void Heartbeat(bool bOn) override;
	virtual void AddCan() override;
	virtual void Celebrate() override;
	virtual void Save(const ss::SaveData& Data) override;
	virtual bool GoOut(const std::string& ActivityId, ss::Chips BuyInCents) override;

	// ss::ui::FrontEndHooks
	virtual void UiSound(ss::SoundId Id, double Volume) override;
	virtual void Continue() override;
	virtual void NewGame(const std::string& ScreenName) override;
	virtual void Resume() override;
	virtual void QuitToMenu() override;
	virtual void QuitGame() override;
	virtual void SettingsChanged(const ss::ui::GameSettings& Settings) override;
};
